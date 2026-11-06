import express from "express";
import session from "express-session";
import createStore from "memorystore";
import { spawn } from "node:child_process";
import { createInterface } from "node:readline";
import { randomBytes } from "node:crypto";
import path from "node:path";
import { fileURLToPath } from "node:url";
const root = path.dirname(fileURLToPath(import.meta.url));
if (!process.env.SGMS_WORKER || !process.env.SGMS_DATA)
  throw Error("请使用项目根目录的启动网页端.ps1");
const child = spawn(process.env.SGMS_WORKER, [], {
  cwd: process.env.SGMS_DATA,
  windowsHide: true,
  shell: false,
  stdio: ["pipe", "pipe", "inherit"],
});
let latestRevision = 1;
let sequence = 0,
  alive = false;
const pending = new Map();
const ready = new Promise((resolve, reject) => {
  child.once("error", reject);
  child.once("exit", () =>
    reject(Error("C数据进程未能启动，请检查目录锁和数据")),
  );
  const lines = createInterface({ input: child.stdout });
  lines.on("line", (line) => {
    try {
      const r = JSON.parse(line);
      if (r.ready) {
        alive = true;
        resolve();
        return;
      }
      if (r.revision) latestRevision = r.revision;
      const item = pending.get(r.id);
      if (item) {
        pending.delete(r.id);
        clearTimeout(item.timer);
        item.resolve(r);
      }
    } catch {
      child.kill();
    }
  });
});
child.on("exit", () => {
  alive = false;
  for (const p of pending.values()) {
    clearTimeout(p.timer);
    p.reject(Error("数据进程已退出，请重启服务并刷新核对操作结果"));
  }
  pending.clear();
});
function rpc(op, actor = "", params = {}, revision = 0) {
  return new Promise((resolve, reject) => {
    if (!alive || pending.size >= 100)
      return reject(Error("数据服务不可用或请求过多"));
    const id = ++sequence;
    const timer = setTimeout(() => {
      pending.delete(id);
      reject(Error("操作结果未确认，请刷新核对"));
      child.kill();
    }, 30000);
    pending.set(id, { resolve, reject, timer });
    child.stdin.write(
      JSON.stringify({ id, op, actor, params, revision }) + "\n",
    );
  });
}
await ready;
const app = express(),
  Store = createStore(session);
const sessionStore = new Store({ checkPeriod: 60000, max: 1000 });
async function revoke(username, except) {
  const sessions = await new Promise((resolve, reject) =>
    sessionStore.all((e, s) => (e ? reject(e) : resolve(s))),
  );
  for (const [id, s] of Object.entries(sessions))
    if (id !== except && (!username || s.user?.username === username))
      await new Promise((resolve) => sessionStore.destroy(id, resolve));
}
app.disable("x-powered-by");
app.use(express.json({ limit: "9mb" }));
app.use(
  session({
    secret: randomBytes(32).toString("hex"),
    resave: false,
    saveUninitialized: false,
    store: sessionStore,
    rolling: true,
    cookie: { httpOnly: true, sameSite: "strict", maxAge: 1800000 },
  }),
);
function validText(v) {
  if (typeof v === "string") return !v.includes("\0");
  if (v && typeof v === "object") return Object.values(v).every(validText);
  return true;
}
app.use("/api", (req, res, next) => {
  res.set("Cache-Control", "no-store");
  if (!validText(req.body))
    return res.status(422).json({ error: "文本不能包含空字符" });
  if (!alive) return res.status(503).json({ error: "数据服务不可用" });
  if (!req.session.csrf) req.session.csrf = randomBytes(24).toString("hex");
  if (req.method !== "GET" && req.get("x-csrf-token") !== req.session.csrf)
    return res.status(403).json({ error: "请求校验失败，请刷新页面" });
  next();
});
const failures = new Map();
app.get("/api/session", (req, res) =>
  res.json({
    user: req.session.user || null,
    csrf: req.session.csrf,
    revision: latestRevision,
    testMode: process.env.SGMS_DEMO === "1",
  }),
);
app.post("/api/login", async (req, res, next) => {
  try {
    const key = req.ip + "|" + String(req.body.username).slice(0, 20),
      f = failures.get(key);
    if (f && f.until > Date.now())
      return res.status(429).json({ error: "登录暂时锁定，请一分钟后重试" });
    const r = await rpc("login", "", req.body);
    if (r.error) {
      const count = (f?.count || 0) + 1;
      if (failures.size > 1000) failures.clear();
      failures.set(key, {
        count: count >= 3 ? 0 : count,
        until: count >= 3 ? Date.now() + 60000 : 0,
      });
      return res.status(401).json(r);
    }
    failures.delete(key);
    await new Promise((resolve, reject) =>
      req.session.regenerate((e) => (e ? reject(e) : resolve())),
    );
    req.session.user = r.data;
    req.session.csrf = randomBytes(24).toString("hex");
    res.json({ ...r, csrf: req.session.csrf });
  } catch (e) {
    next(e);
  }
});
app.post("/api/logout", (req, res) =>
  req.session.destroy(() => res.json({ data: true })),
);
app.use("/api", (req, res, next) =>
  req.session.user ? next() : res.status(401).json({ error: "请先登录" }),
);
app.get("/api/snapshot", async (req, res, next) => {
  try {
    res.json(await rpc("snapshot", req.session.user.username));
  } catch (e) {
    next(e);
  }
});
app.post("/api/action/:op", async (req, res, next) => {
  try {
    if (
      !["student", "teacher", "class", "grade", "password", "seed"].includes(
        req.params.op,
      )
    )
      return res.status(404).json({ error: "操作不存在" });
    const r = await rpc(
      req.params.op,
      req.session.user.username,
      req.body.params,
      req.body.revision,
    );
    if (!r.error && req.params.op === "password") {
      req.session.user.mustChangePassword = false;
      await revoke(req.session.user.username, req.sessionID);
    }
    res.status(r.error ? 409 : 200).json(r);
  } catch (e) {
    next(e);
  }
});
import { readFile } from "node:fs/promises";
function parseCSV(text) {
  if (Buffer.byteLength(text) > 8 * 1024 * 1024) throw Error("CSV 超过8 MiB");
  text = text.replace(/^\uFEFF/, "");
  const rows = [];
  let row = [],
    value = "",
    quoted = false,
    closed = false,
    line = 1,
    start = 1;
  for (let i = 0; i <= text.length; i++) {
    const c = text[i];
    if (quoted) {
      if (c === '"') {
        if (text[i + 1] === '"') {
          value += '"';
          i++;
        } else {
          quoted = false;
          closed = true;
        }
      } else if (c === undefined) throw Error("CSV 引号未闭合");
      else {
        value += c;
        if (c === "\n") line++;
      }
    } else if (c === '"' && !value && !closed) quoted = true;
    else if (c === "," || c === "\n" || c === undefined) {
      row.push(value.replace(/\r$/, ""));
      value = "";
      closed = false;
      if (c !== ",") {
        if (row.some((x) => x !== "")) rows.push({ line: start, fields: row });
        row = [];
        if (c === "\n") line++;
        start = line;
      }
    } else {
      if (closed) throw Error("CSV 闭合引号后有非法字符");
      value += c;
    }
    if (Buffer.byteLength(value) > 511)
      throw Error(`第${line}行字段超过511字节`);
  }
  return rows;
}
app.post("/api/csv/preview", async (req, res, next) => {
  try {
    if (req.session.user.role !== 1)
      return res.status(403).json({ error: "只有教师可以导入" });
    const { text, classId } = req.body;
    const parsed = parseCSV(text);
    const snap = await rpc("snapshot", req.session.user.username);
    const course = req.session.user.course;
    const errors = [],
      rows = [],
      seen = new Set();
    let columns = 0;
    for (const r of parsed) {
      const f = r.fields;
      columns ||= f.length;
      if (
        rows.length === 0 &&
        errors.length === 0 &&
        ["学号", "stuNum"].includes(f[0]) &&
        ["成绩", "score"].includes(f.at(-1))
      )
        continue;
      const id = f[0],
        name = f.length === 4 ? f[1] : "",
        co = f.at(-2),
        raw = f.at(-1),
        s = snap.data.students.find((s) => s.id === id);
      const reason =
        f.length !== columns || ![3, 4].includes(f.length)
          ? "列数错误"
          : !s
            ? "学生不存在或无授课权限"
            : s.classId !== classId
              ? "不属于所选班级"
              : snap.data.classes.find((c) => c.id === classId)?.inactive
                ? "班级停用"
                : name && name !== s.name
                  ? "姓名不匹配"
                  : co !== course
                    ? "不是任教科目"
                    : seen.has(id)
                      ? "重复学生"
                      : s.grades.some((g) => g.course === co)
                        ? "已有成绩，不能覆盖"
                        : !/^\s*[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?\s*$/.test(
                              raw,
                            ) ||
                            !Number.isFinite(Number(raw)) ||
                            Number(raw) < 0 ||
                            Number(raw) > 100
                          ? "成绩无效"
                          : null;
      seen.add(id);
      if (reason) errors.push({ line: r.line, error: reason });
      else rows.push({ id, name, course: co, score: Number(raw) });
    }
    if (!rows.length && !errors.length)
      errors.push({ line: 1, error: "没有成绩记录" });
    const token = randomBytes(24).toString("hex");
    req.session.preview = {
      token,
      rows,
      errors,
      classId,
      revision: snap.revision,
      expires: Date.now() + 120000,
    };
    res.json({ data: { token, rows, errors } });
  } catch (e) {
    next(e);
  }
});
app.post("/api/csv/commit", async (req, res, next) => {
  try {
    const p = req.session.preview;
    req.session.preview = null;
    if (
      !p ||
      p.errors.length ||
      p.token !== req.body.token ||
      p.expires < Date.now()
    )
      return res.status(409).json({ error: "预览已过期，请重新上传" });
    const r = await rpc(
      "batch",
      req.session.user.username,
      { rows: p.rows, classId: p.classId },
      p.revision,
    );
    res.status(r.error ? 409 : 200).json(r);
  } catch (e) {
    next(e);
  }
});
app.get("/api/export", async (req, res, next) => {
  try {
    if (req.session.user.role === 2)
      return res.status(403).json({ error: "无权导出" });
    const r = await rpc("snapshot", req.session.user.username);
    if (r.error) return res.status(403).json(r);
    const esc = (v) => '"' + String(v).replaceAll('"', '""') + '"';
    let csv = "\uFEFF学号,科目,成绩\r\n",
      count = 0;
    for (const s of r.data.students) {
      if (
        req.query.classId !== undefined &&
        s.classId !== Number(req.query.classId)
      )
        continue;
      for (const g of s.grades) {
        if (req.session.user.role === 1 && g.course !== req.session.user.course)
          continue;
        csv += [s.id, g.course, g.score].map(esc).join(",") + "\r\n";
        count++;
      }
    }
    if (!count) return res.status(422).json({ error: "没有可导出的成绩" });
    res.attachment("grades.csv").type("text/csv").send(csv);
  } catch (e) {
    next(e);
  }
});
app.post("/api/danger/preview", async (req, res, next) => {
  try {
    if (
      req.session.user.role !== 0 ||
      !["delete", "clear"].includes(req.body.op)
    )
      return res.status(403).json({ error: "无权操作" });
    const r = await rpc("snapshot", req.session.user.username);
    if (r.error) return res.status(403).json(r);
    const s = r.data.students.find((s) => s.id === req.body.id);
    if (req.body.op === "delete" && !s)
      return res.status(422).json({ error: "学生不存在" });
    const token = randomBytes(24).toString("hex");
    req.session.danger = {
      token,
      op: req.body.op,
      id: req.body.id,
      revision: r.revision,
      expires: Date.now() + 120000,
    };
    res.json({
      data: {
        token,
        message:
          req.body.op === "delete"
            ? `将删除 ${s.id} ${s.name} 的档案、成绩、账号和班级关联。`
            : `将清空 ${r.data.students.length} 名学生及所有教师，仅保留管理员和班级定义。`,
      },
    });
  } catch (e) {
    next(e);
  }
});
app.post("/api/danger/commit", async (req, res, next) => {
  try {
    const p = req.session.danger;
    req.session.danger = null;
    if (!p || p.token !== req.body.token || p.expires < Date.now())
      return res.status(409).json({ error: "确认已过期" });
    const result = await rpc(
      p.op,
      req.session.user.username,
      { id: p.id, password: req.body.password, confirm: req.body.confirm },
      p.revision,
    );
    if (!result.error)
      await revoke(p.op === "delete" ? p.id : null, req.sessionID);
    res.json(result);
  } catch (e) {
    next(e);
  }
});
app.get("/api/audit", async (req, res, next) => {
  try {
    if (req.session.user.role !== 0 || req.session.user.mustChangePassword)
      return res.status(403).json({ error: "无权查看日志" });
    const marker = await readFile(
      path.join(process.env.SGMS_DATA, ".sgms/current.dat"),
    );
    const length = marker.readUInt32LE(12),
      generation = marker.subarray(16, 16 + length).toString("utf8");
    if (!/^[a-f0-9]{32}$/.test(generation)) throw Error("数据标记无效");
    const log = await readFile(
      path.join(process.env.SGMS_DATA, ".sgms", generation, "audit.dat"),
      "utf8",
    );
    res.json({ data: log.slice(-200000) });
  } catch (e) {
    next(e);
  }
});

app.use(express.static(path.join(root, "dist")));
app.get("/", (req, res) => res.sendFile(path.join(root, "dist/index.html")));
app.use((e, req, res, next) => {
  console.error(e.message);
  res.status(e.status || 503).json({ error: "服务异常：" + e.message });
});
const server = app.listen(
  Number(process.env.PORT || 3000),
  process.env.HOST || "127.0.0.1",
  () =>
    console.log(
      `学生成绩管理系统：http://localhost:${process.env.PORT || 3000} （数据：${process.env.SGMS_DATA}）`,
    ),
);
process.on("SIGINT", () => {
  server.close();
  child.stdin.end();
});
