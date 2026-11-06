import { BookIcon, Status, MyScores } from "./components";
import React, { useState, useEffect } from "react";
import { createRoot } from "react-dom/client";
import type { User, Student, Snapshot, Result } from "./types";
import "./style.css";
let csrf = "";
async function api<T>(url: string, body?: unknown): Promise<Result<T>> {
  const r = await fetch("/api/" + url, {
    method: body ? "POST" : "GET",
    headers: { "Content-Type": "application/json", "x-csrf-token": csrf },
    body: body ? JSON.stringify(body) : undefined,
  });
  const j = await r.json();
  if (j.csrf) csrf = j.csrf;
  if (j.error) throw Error(j.error);
  return j;
}
const names = ["管理员", "教师", "学生"];
function App() {
  const [user, setUser] = useState<User | null>(null),
    [ready, setReady] = useState(false),
    [data, setData] = useState<Snapshot>({
      students: [],
      classes: [],
      users: [],
    }),
    [rev, setRev] = useState(0),
    [page, setPage] = useState(""),
    [error, setError] = useState(""),
    [note, setNote] = useState(""),
    [query, setQuery] = useState(""),
    [cls, setCls] = useState(""),
    [course, setCourse] = useState(""),
    [index, setIndex] = useState(1),
    [drawer, setDrawer] = useState<{ op: string; value: any } | null>(null),
    [busy, setBusy] = useState(false),
    [menu, setMenu] = useState(false);
  const nav =
    user?.role === 0
      ? [
          "系统首页",
          "学生管理",
          "教师管理",
          "班级管理",
          "成绩查询",
          "统计分析",
          "系统管理",
          "账号设置",
        ]
      : user?.role === 1
        ? ["成绩管理", "班级统计", "成绩排名", "授权学生", "账号设置"]
        : ["我的成绩", "我的班级", "账号设置"];
  async function refresh() {
    try {
      const r = await api<Snapshot>("snapshot");
      setData(r.data);
      setRev(r.revision);
    } catch (e) {
      setError(String(e));
    }
  }
  useEffect(() => {
    api<any>("session")
      .then((r) => {
        setRev(r.revision);
        setUser((r as any).user);
        setReady(true);
      })
      .catch((e) => {
        setError(String(e));
        setReady(true);
      });
  }, []);
  useEffect(() => {
    if (user) {
      setPage(
        user.mustChangePassword
          ? "账号设置"
          : user.role === 0
            ? "系统首页"
            : user.role === 1
              ? "成绩管理"
              : "我的成绩",
      );
      if (!user.mustChangePassword) void refresh();
    }
  }, [user]);
  async function action(op: string, params: any) {
    setBusy(true);
    setError("");
    try {
      await api("action/" + op, { params, revision: rev });
      setDrawer(null);
      setNote("操作已保存");
      if (op === "password") setUser({ ...user!, mustChangePassword: false });
      await refresh();
    } catch (e) {
      setError(String(e));
      await refresh();
    } finally {
      setBusy(false);
    }
  }
  const className = (id: number) =>
    data.classes.find((c) => c.id === id)?.name || "未分班";
  const selectedCourse = user?.role === 1 ? user.course : course;
  const courses = [
    ...new Set(data.students.flatMap((s) => s.grades.map((g) => g.course))),
  ];
  const rows = data.students.filter(
    (s) =>
      (!cls || String(s.classId) === cls) &&
      (!query || s.id.includes(query) || s.name.includes(query)),
  );
  const score = (s: Student) =>
    s.grades.find((g) => g.course === selectedCourse)?.score ?? null;
  function changePage(n: string) {
    setPage(n);
    setIndex(1);
    setQuery("");
    setCls("");
    setMenu(false);
    setNote("");
  }
  if (!ready) return <main>正在连接数据服务…</main>;
  if (!user)
    return (
      <div className="login">
        <form
          onSubmit={async (e) => {
            e.preventDefault();
            setBusy(true);
            try {
              const f = new FormData(e.currentTarget);
              const r = await api<User>("login", Object.fromEntries(f));
              setRev(r.revision);
              setUser(r.data);
            } catch (e) {
              setError(String(e));
            } finally {
              setBusy(false);
            }
          }}
        >
          <div className="mark">
            <BookIcon />
          </div>
          <h1>学生成绩管理系统</h1>
          <p>使用管理员账号、教师工号或学生学号登录</p>
          <label>
            账号
            <input
              name="username"
              required
              autoComplete="username"
              maxLength={19}
            />
          </label>
          <label>
            密码
            <input
              name="password"
              type="password"
              required
              autoComplete="current-password"
            />
          </label>
          {error && (
            <p role="alert" className="error">
              {error}
            </p>
          )}
          <button disabled={busy}>登录</button>
          <small>学生初始密码与学号相同；管理员首次登录须修改默认密码。</small>
        </form>
      </div>
    );
  const filter = (
    <div className="filters">
      <label>
        班级
        <select
          value={cls}
          onChange={(e) => {
            setCls(e.target.value);
            setIndex(1);
          }}
        >
          <option value="">全部班级</option>
          {data.classes.map((c) => (
            <option key={c.id} value={c.id}>
              {c.name}
              {c.inactive ? "（停用）" : ""}
            </option>
          ))}
        </select>
      </label>
      {user.role !== 2 && page !== "学生管理" && (
        <label>
          科目
          {user.role === 1 ? (
            <input disabled value={user.course} />
          ) : (
            <select value={course} onChange={(e) => setCourse(e.target.value)}>
              <option value="">全部科目</option>
              {courses.map((c) => (
                <option key={c}>{c}</option>
              ))}
            </select>
          )}
        </label>
      )}
      <input
        aria-label="搜索学号或姓名"
        placeholder="搜索学号或姓名"
        value={query}
        onChange={(e) => {
          setQuery(e.target.value);
          setIndex(1);
        }}
      />
      <button
        className="secondary"
        onClick={() => {
          setQuery("");
          setCls("");
          setCourse("");
        }}
      >
        重置
      </button>
    </div>
  );
  const settings = page === "账号设置";
  const stats = ["统计分析", "班级统计", "成绩排名"].includes(page);
  const studentTable = [
    "学生管理",
    "成绩查询",
    "成绩管理",
    "授权学生",
    "unused",
  ].includes(page);
  return (
    <>
      <aside className={menu ? "open" : ""}>
        <div className="brand">
          <span>
            <BookIcon />
          </span>
          学生成绩
          <br />
          管理系统
        </div>
        <p>{names[user.role]}工作台</p>
        <nav>
          {nav.map((n) => (
            <button
              key={n}
              className={page === n ? "active" : ""}
              disabled={user.mustChangePassword && n !== "账号设置"}
              onClick={() => changePage(n)}
            >
              {n}
            </button>
          ))}
        </nav>
        <footer>
          <strong>{user.name}</strong>
          <small>
            {user.username} {user.course}
          </small>
          <button
            className="secondary"
            onClick={async () => {
              await api("logout", {});
              await api("session");
              setUser(null);
              setData({ students: [], classes: [], users: [] });
            }}
          >
            退出登录
          </button>
        </footer>
      </aside>
      <div className="workspace">
        <header>
          <button
            className="mobile secondary"
            aria-label="打开导航"
            onClick={() => setMenu(!menu)}
          >
            ☰
          </button>
          {names[user.role]}工作台 / {page}
          <span>{user.name}</span>
        </header>
        <main>
          <div className="title">
            <div>
              <h1>{page}</h1>
              <p>
                {user.role === 1
                  ? "管理任教科目成绩，查看授权学生的学习情况"
                  : user.role === 2
                    ? "查看本人学习成绩与班级信息"
                    : "维护档案与授课关系，查询全校成绩"}
              </p>
            </div>
            <div className="actions">
              {page === "学生管理" && (
                <button
                  onClick={() =>
                    setDrawer({
                      op: "student",
                      value: {
                        id: "",
                        name: "",
                        classId:
                          data.classes.find((c) => !c.inactive)?.id ?? -1,
                        teachers: [],
                      },
                    })
                  }
                >
                  新增学生
                </button>
              )}
              {page === "教师管理" && (
                <button
                  onClick={() =>
                    setDrawer({
                      op: "teacher",
                      value: { id: "", name: "", course: "", password: "" },
                    })
                  }
                >
                  新增教师
                </button>
              )}
              {page === "班级管理" && (
                <button
                  onClick={() =>
                    setDrawer({
                      op: "class",
                      value: { id: -1, name: "", inactive: false },
                    })
                  }
                >
                  新增班级
                </button>
              )}
              {page === "成绩管理" && (
                <>
                  <button
                    onClick={() =>
                      setDrawer({
                        op: "grade",
                        value: { id: rows[0]?.id || "", score: "" },
                      })
                    }
                  >
                    录入成绩
                  </button>
                  <label className="fileButton">
                    导入 CSV
                    <input
                      type="file"
                      accept=".csv"
                      onChange={async (e) => {
                        const f = e.target.files?.[0];
                        if (!f) return;
                        try {
                          if (!cls) throw Error("请先选择班级");
                          const text = await f.text();
                          const r = await api<any>("csv/preview", {
                            text,
                            classId: Number(cls),
                          });
                          setDrawer({ op: "csv", value: r.data });
                        } catch (e) {
                          setError(String(e));
                        }
                        e.target.value = "";
                      }}
                    />
                  </label>
                </>
              )}
              {["成绩查询", "成绩管理"].includes(page) && (
                <button
                  className="secondary"
                  onClick={async () => {
                    try {
                      const r = await fetch(
                        "/api/export" + (cls ? "?classId=" + cls : ""),
                      );
                      if (!r.ok) throw Error("导出失败");
                      const a = document.createElement("a");
                      a.href = URL.createObjectURL(await r.blob());
                      a.download = "成绩.csv";
                      a.click();
                      URL.revokeObjectURL(a.href);
                    } catch (e) {
                      setError(String(e));
                    }
                  }}
                >
                  导出 CSV
                </button>
              )}
              <button
                className="secondary"
                disabled={busy || user.mustChangePassword}
                onClick={() => void refresh()}
              >
                刷新
              </button>
            </div>
          </div>
          {error && (
            <div className="error" role="alert">
              {error}
            </div>
          )}
          {note && (
            <div className="success" role="status">
              {note}
            </div>
          )}
          {page === "系统首页" && (
            <>
              <div className="summary">
                {[
                  ["学生", data.students.length],
                  ["教师", data.users.filter((u) => u.role === 1).length],
                  ["班级", data.classes.length],
                  [
                    "成绩记录",
                    data.students.reduce((n, s) => n + s.grades.length, 0),
                  ],
                ].map(([k, v]) => (
                  <div key={k}>
                    <small>{k}</small>
                    <strong>{v}</strong>
                  </div>
                ))}
              </div>
              <h2>管理入口</h2>
              <div className="quick">
                {nav.slice(1, 7).map((n) => (
                  <button
                    className="secondary"
                    key={n}
                    onClick={() => changePage(n)}
                  >
                    {n}
                  </button>
                ))}
              </div>
            </>
          )}
          {page === "我的成绩" &&
            rows.map((s) => (
              <MyScores
                key={s.id}
                student={s}
                className={className(s.classId)}
              />
            ))}
          {studentTable && page !== "我的成绩" && (
            <>
              {user.role !== 2 && filter}
              {user.role === 1 && (
                <div className="summary">
                  <div>
                    <small>当前筛选学生</small>
                    <strong>{rows.length}</strong>
                  </div>
                  <div>
                    <small>已录入</small>
                    <strong>
                      {rows.filter((s) => score(s) !== null).length}
                    </strong>
                  </div>
                  <div>
                    <small>未录入</small>
                    <strong>
                      {rows.filter((s) => score(s) === null).length}
                    </strong>
                  </div>
                  <div>
                    <small>当前科目</small>
                    <strong>{user.course}</strong>
                  </div>
                </div>
              )}
              <section className="table">
                <table>
                  <thead>
                    <tr>
                      <th>学号</th>
                      <th>姓名</th>
                      <th>班级</th>
                      <th>
                        {page === "学生管理"
                          ? "授课教师"
                          : selectedCourse
                            ? selectedCourse + "成绩"
                            : "完整成绩"}
                      </th>
                      {user.role === 1 && <th>状态</th>}
                      {user.role !== 2 && <th>操作</th>}
                    </tr>
                  </thead>
                  <tbody>
                    {rows.slice((index - 1) * 10, index * 10).map((s) => (
                      <tr key={s.id}>
                        <td>{s.id}</td>
                        <td>
                          <button
                            className="link"
                            onClick={() =>
                              setDrawer({ op: "detail", value: s })
                            }
                          >
                            {s.name}
                          </button>
                        </td>
                        <td>{className(s.classId)}</td>
                        <td>
                          {page === "学生管理" ? (
                            s.teachers
                              .map(
                                (t) =>
                                  data.users.find((u) => u.username === t)
                                    ?.name || t,
                              )
                              .join("、")
                          ) : selectedCourse ? (
                            <span
                              className={
                                score(s) !== null && score(s)! < 60
                                  ? "fail"
                                  : ""
                              }
                            >
                              {score(s) ?? "未录入"}
                            </span>
                          ) : s.grades.length ? (
                            s.grades
                              .map((g) => `${g.course} ${g.score}`)
                              .join(" / ")
                          ) : (
                            "未录入"
                          )}
                        </td>
                        {user.role === 1 && (
                          <td>
                            <Status value={score(s)} />
                          </td>
                        )}
                        {user.role !== 2 && (
                          <td>
                            {user.role === 0 && page === "学生管理" ? (
                              <button
                                className="link"
                                onClick={() =>
                                  setDrawer({ op: "student", value: { ...s } })
                                }
                              >
                                编辑
                              </button>
                            ) : user.role === 1 ? (
                              <button
                                className="link"
                                onClick={() =>
                                  setDrawer({
                                    op: "grade",
                                    value: { id: s.id, score: score(s) ?? "" },
                                  })
                                }
                              >
                                录入 / 修改
                              </button>
                            ) : (
                              <button
                                className="link"
                                onClick={() =>
                                  setDrawer({ op: "detail", value: s })
                                }
                              >
                                查看
                              </button>
                            )}
                          </td>
                        )}
                      </tr>
                    ))}
                  </tbody>
                </table>
                {!rows.length && (
                  <p className="empty">
                    没有符合条件的学生。教师请联系管理员分配授课关系。
                  </p>
                )}
                <div className="pagination">
                  共 {rows.length} 名学生
                  <span>
                    <button
                      className="secondary"
                      disabled={index === 1}
                      onClick={() => setIndex(index - 1)}
                    >
                      上一页
                    </button>{" "}
                    {index}{" "}
                    <button
                      className="secondary"
                      disabled={index * 10 >= rows.length}
                      onClick={() => setIndex(index + 1)}
                    >
                      下一页
                    </button>
                  </span>
                </div>
              </section>
            </>
          )}
          {page === "我的班级" &&
            data.classes.map((c) => (
              <section className="panel" key={c.id}>
                <h2>{c.name}</h2>
                <p>状态：{c.inactive ? "停用" : "启用"}</p>
                <p>班级由管理员统一分配。</p>
              </section>
            ))}
          {page === "教师管理" && (
            <section className="table">
              <table>
                <thead>
                  <tr>
                    <th>工号</th>
                    <th>姓名</th>
                    <th>科目</th>
                    <th>操作</th>
                  </tr>
                </thead>
                <tbody>
                  {data.users
                    .filter((u) => u.role === 1)
                    .map((u) => (
                      <tr key={u.username}>
                        <td>{u.username}</td>
                        <td>{u.name}</td>
                        <td>{u.course}</td>
                        <td>
                          <button
                            className="link"
                            onClick={() =>
                              setDrawer({
                                op: "teacher",
                                value: {
                                  id: u.username,
                                  name: u.name,
                                  course: u.course,
                                  password: "",
                                },
                              })
                            }
                          >
                            编辑姓名
                          </button>
                        </td>
                      </tr>
                    ))}
                </tbody>
              </table>
            </section>
          )}
          {page === "班级管理" && (
            <section className="table">
              <table>
                <thead>
                  <tr>
                    <th>班级</th>
                    <th>人数</th>
                    <th>状态</th>
                    <th>操作</th>
                  </tr>
                </thead>
                <tbody>
                  {data.classes.map((c) => (
                    <tr key={c.id}>
                      <td>{c.name}</td>
                      <td>
                        {data.students.filter((s) => s.classId === c.id).length}
                      </td>
                      <td>{c.inactive ? "停用" : "启用"}</td>
                      <td>
                        <button
                          className="link"
                          onClick={() =>
                            setDrawer({ op: "class", value: { ...c } })
                          }
                        >
                          重命名 / 状态
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </section>
          )}
          {stats && (
            <>
              {filter}
              {!selectedCourse ? (
                <p>请选择统计科目。</p>
              ) : (
                (() => {
                  const valid = rows
                      .filter((s) => score(s) !== null)
                      .sort(
                        (a, b) =>
                          score(b)! - score(a)! || a.id.localeCompare(b.id),
                      ),
                    values = valid.map((s) => score(s)!);
                  return (
                    <>
                      <div className="summary">
                        {[
                          [
                            "平均分",
                            values.length
                              ? (
                                  values.reduce((a, b) => a + b, 0) /
                                  values.length
                                ).toFixed(2)
                              : "--",
                          ],
                          [
                            "最高分",
                            values.length ? Math.max(...values) : "--",
                          ],
                          [
                            "最低分",
                            values.length ? Math.min(...values) : "--",
                          ],
                          [
                            "及格率",
                            values.length
                              ? (
                                  (values.filter((v) => v >= 60).length /
                                    values.length) *
                                  100
                                ).toFixed(2) + "%"
                              : "--",
                          ],
                          ["未录入", rows.length - values.length],
                        ].map(([k, v]) => (
                          <div key={k}>
                            <small>{k}</small>
                            <strong>{v}</strong>
                          </div>
                        ))}
                      </div>
                      <section className="table">
                        <table>
                          <thead>
                            <tr>
                              <th>排名</th>
                              <th>学号</th>
                              <th>姓名</th>
                              <th>成绩</th>
                            </tr>
                          </thead>
                          <tbody>
                            {valid.map((s, i) => (
                              <tr key={s.id}>
                                <td>
                                  {valid.findIndex(
                                    (t) => score(t) === score(s),
                                  ) + 1}
                                </td>
                                <td>{s.id}</td>
                                <td>{s.name}</td>
                                <td>{score(s)}</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </section>
                    </>
                  );
                })()
              )}
            </>
          )}
          {settings && (
            <form
              className="panel form"
              onSubmit={(e) => {
                e.preventDefault();
                const f = Object.fromEntries(new FormData(e.currentTarget));
                if (f.password !== f.again) {
                  setError("两次密码不一致");
                  return;
                }
                void action("password", f);
              }}
            >
              {user.mustChangePassword && (
                <p className="error">首次登录必须修改默认管理员密码。</p>
              )}
              <label>
                当前密码
                <input
                  name="oldPassword"
                  type="password"
                  required
                  autoComplete="current-password"
                />
              </label>
              <label>
                新密码（6–19字节）
                <input
                  name="password"
                  type="password"
                  required
                  autoComplete="new-password"
                />
              </label>
              <label>
                确认新密码
                <input
                  name="again"
                  type="password"
                  required
                  autoComplete="new-password"
                />
              </label>
              <button disabled={busy}>保存密码</button>
            </form>
          )}
          {page === "系统管理" && (
            <>
              <section className="panel">
                <h2>示例数据</h2>
                <p>补充虚构教师、班级、学生及成绩，保留已有密码和成绩。</p>
                <button onClick={() => setDrawer({ op: "seed", value: {} })}>
                  预览并补充示例数据
                </button>
              </section>
              <section className="panel">
                <h2>用户账号</h2>
                {data.users.map((u) => (
                  <p key={u.username}>
                    {u.username}　{u.name}　{names[u.role]}
                  </p>
                ))}
              </section>
              <section className="panel">
                <h2>操作日志</h2>
                <button
                  className="secondary"
                  onClick={async () => {
                    try {
                      const r = await api<string>("audit");
                      setDrawer({ op: "audit", value: r.data });
                    } catch (e) {
                      setError(String(e));
                    }
                  }}
                >
                  查看日志
                </button>
              </section>
              <section className="panel danger">
                <h2>危险操作</h2>
                <p>操作会影响学生档案及关联数据，请先核对影响范围。</p>
                <button
                  className="secondary"
                  onClick={() =>
                    setDrawer({
                      op: "delete",
                      value: { id: "", password: "", confirm: "" },
                    })
                  }
                >
                  删除单个学生
                </button>{" "}
                <button
                  className="secondary"
                  onClick={() =>
                    setDrawer({
                      op: "clear",
                      value: { password: "", confirm: "" },
                    })
                  }
                >
                  清空学生和教师
                </button>
              </section>
            </>
          )}
        </main>
      </div>
      {drawer && (
        <div className="overlay">
          <section
            className="drawer"
            role="dialog"
            aria-modal="true"
            aria-label="操作表单"
          >
            <button
              className="close secondary"
              aria-label="关闭"
              onClick={() => setDrawer(null)}
            >
              ×
            </button>
            <h2>
              {
                (
                  {
                    student: "学生档案",
                    teacher: "教师档案",
                    class: "班级设置",
                    grade: "录入 / 修改成绩",
                    detail: "学生完整成绩",
                    seed: "补充示例数据",
                    csv: "CSV 导入预览",
                    delete: "删除学生",
                    clear: "清空用户",
                    audit: "操作日志",
                  } as any
                )[drawer.op]
              }
            </h2>
            {error && (
              <p className="error" role="alert">
                {error}
              </p>
            )}
            {drawer.op === "detail" ? (
              <>
                <p>
                  {drawer.value.id}　{drawer.value.name}
                </p>
                <p>{className(drawer.value.classId)}</p>
                {drawer.value.grades.map((g: any) => (
                  <p key={g.course}>
                    {g.course}：{g.score}
                  </p>
                ))}
                <p>
                  平均分：
                  {drawer.value.grades.length
                    ? (
                        drawer.value.grades.reduce(
                          (n: number, g: any) => n + g.score,
                          0,
                        ) / drawer.value.grades.length
                      ).toFixed(2)
                    : "--"}
                </p>
                <small>未录入成绩不计入平均分。</small>
              </>
            ) : drawer.op === "audit" ? (
              <pre>{drawer.value}</pre>
            ) : drawer.op === "csv" ? (
              <>
                <p>共 {drawer.value.rows?.length || 0} 条记录</p>
                <pre>
                  {JSON.stringify(
                    drawer.value.errors?.length
                      ? drawer.value.errors
                      : drawer.value.rows,
                    null,
                    2,
                  )}
                </pre>
                {!drawer.value.errors?.length && (
                  <button
                    disabled={busy}
                    onClick={async () => {
                      setBusy(true);
                      try {
                        await api("csv/commit", { token: drawer.value.token });
                        setDrawer(null);
                        setNote("CSV 已整批导入");
                        await refresh();
                      } catch (e) {
                        setError(String(e));
                      } finally {
                        setBusy(false);
                      }
                    }}
                  >
                    确认导入
                  </button>
                )}
              </>
            ) : (
              <form
                onSubmit={async (e) => {
                  e.preventDefault();
                  if (["delete", "clear"].includes(drawer.op)) {
                    try {
                      const p = await api<any>("danger/preview", {
                        op: drawer.op,
                        id: drawer.value.id,
                      });
                      if (!window.confirm(p.data.message)) return;
                      setBusy(true);
                      await api("danger/commit", {
                        token: p.data.token,
                        password: drawer.value.password,
                        confirm: drawer.value.confirm,
                      });
                      setDrawer(null);
                      await refresh();
                    } catch (e) {
                      setError(String(e));
                    } finally {
                      setBusy(false);
                    }
                    return;
                  }
                  if (!window.confirm("确认保存以上信息？")) return;
                  void action(drawer.op, drawer.value);
                }}
              >
                {drawer.op === "seed" ? (
                  <p>
                    添加3位教师、3个班级、30名学生及87条成绩；如有冲突将整批拒绝。
                  </p>
                ) : (
                  Object.entries(drawer.value)
                    .filter(([k]) => !["grades", "teachers"].includes(k))
                    .map(([k, v]) => (
                      <label key={k}>
                        {(
                          {
                            id: "学号 / 工号",
                            name: "姓名 / 班级名称",
                            course: "任教科目",
                            classId: "班级",
                            inactive: "停用班级",
                            score: "成绩（0–100）",
                            password: "密码",
                            confirm:
                              drawer.op === "clear"
                                ? "输入：清空全部用户"
                                : "输入要删除的完整学号",
                          } as any
                        )[k] || k}
                        {k === "classId" ? (
                          <select
                            value={String(v)}
                            onChange={(e) =>
                              setDrawer({
                                ...drawer,
                                value: {
                                  ...drawer.value,
                                  [k]: Number(e.target.value),
                                },
                              })
                            }
                          >
                            {data.classes
                              .filter((c) => !c.inactive)
                              .map((c) => (
                                <option key={c.id} value={c.id}>
                                  {c.name}
                                </option>
                              ))}
                          </select>
                        ) : k === "inactive" ? (
                          <input
                            type="checkbox"
                            checked={Boolean(v)}
                            onChange={(e) =>
                              setDrawer({
                                ...drawer,
                                value: {
                                  ...drawer.value,
                                  [k]: e.target.checked,
                                },
                              })
                            }
                          />
                        ) : k === "id" && drawer.op === "class" ? (
                          <small>
                            {Number(v) < 0 ? "新建班级" : "现有班级"}
                          </small>
                        ) : (
                          <input
                            required={
                              !(
                                k === "password" &&
                                drawer.op === "teacher" &&
                                data.users.some(
                                  (u) => u.username === drawer.value.id,
                                )
                              )
                            }
                            type={
                              k === "password"
                                ? "password"
                                : k === "score"
                                  ? "number"
                                  : "text"
                            }
                            step="any"
                            value={String(v)}
                            onChange={(e) =>
                              setDrawer({
                                ...drawer,
                                value: {
                                  ...drawer.value,
                                  [k]:
                                    k === "score"
                                      ? Number(e.target.value)
                                      : e.target.value,
                                },
                              })
                            }
                          />
                        )}
                      </label>
                    ))
                )}
                {drawer.op === "student" && (
                  <fieldset>
                    <legend>授课教师</legend>
                    {data.users
                      .filter((u) => u.role === 1)
                      .map((u) => (
                        <label key={u.username} className="check">
                          <input
                            type="checkbox"
                            checked={drawer.value.teachers.includes(u.username)}
                            onChange={(e) =>
                              setDrawer({
                                ...drawer,
                                value: {
                                  ...drawer.value,
                                  teachers: e.target.checked
                                    ? [...drawer.value.teachers, u.username]
                                    : drawer.value.teachers.filter(
                                        (t: string) => t !== u.username,
                                      ),
                                },
                              })
                            }
                          />
                          {u.name} · {u.course}
                        </label>
                      ))}
                    <small>学生初始密码与学号相同。</small>
                  </fieldset>
                )}
                <div className="actions">
                  <button
                    type="button"
                    className="secondary"
                    onClick={() => setDrawer(null)}
                  >
                    取消
                  </button>
                  <button disabled={busy}>预览并确认</button>
                </div>
              </form>
            )}
          </section>
        </div>
      )}
    </>
  );
}
createRoot(document.getElementById("root")!).render(<App />);
