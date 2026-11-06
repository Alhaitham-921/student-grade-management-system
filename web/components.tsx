import React from "react";
export function BookIcon() {
  return (
    <svg
      width="40"
      height="40"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <path d="M12 6c-3-3-7-3-10-2v16c4-1 7-1 10 2 3-3 6-3 10-2V4c-3-1-7-1-10 2Z" />
      <path d="M12 6v16" />
    </svg>
  );
}
export function Status({ value }: { value: number | null }) {
  return (
    <span
      className={
        "status " + (value === null ? "missing" : value < 60 ? "low" : "pass")
      }
    >
      {value === null ? "未录入" : value < 60 ? "不及格" : "已录入"}
    </span>
  );
}
export function StudentDetail({
  student,
  className,
}: {
  student: {
    id: string;
    name: string;
    grades: { course: string; score: number }[];
  };
  className: string;
}) {
  return (
    <>
      <p>
        {student.id}　{student.name}
      </p>
      <p>{className}</p>
      {student.grades.map((g) => (
        <p key={g.course}>
          {g.course}：{g.score}
        </p>
      ))}
      <p>
        平均分：
        {student.grades.length
          ? (
              student.grades.reduce((n, g) => n + g.score, 0) /
              student.grades.length
            ).toFixed(2)
          : "--"}
      </p>
      <small>未录入成绩不计入平均分。</small>
    </>
  );
}
export function MyScores({
  student,
  className,
}: {
  student: import("./types").Student;
  className: string;
}) {
  const n = student.grades.length;
  return (
    <>
      <p>
        {student.name}　{student.id}　{className}
      </p>
      <div className="summary">
        <div>
          <small>已录入科目</small>
          <strong>{n}</strong>
        </div>
        <div>
          <small>平均分</small>
          <strong>
            {n
              ? (student.grades.reduce((s, g) => s + g.score, 0) / n).toFixed(2)
              : "--"}
          </strong>
        </div>
      </div>
      <section className="table">
        <table>
          <thead>
            <tr>
              <th>科目</th>
              <th>成绩</th>
              <th>状态</th>
            </tr>
          </thead>
          <tbody>
            {student.grades.map((g) => (
              <tr key={g.course}>
                <td>{g.course}</td>
                <td>{g.score}</td>
                <td>
                  <Status value={g.score} />
                </td>
              </tr>
            ))}
          </tbody>
        </table>
        {!n && <p className="empty">暂无成绩，请等待教师录入。</p>}
      </section>
      <p>未录入成绩不计入平均分。</p>
      <section className="panel">
        <h2>我的班级</h2>
        <p>{className}</p>
        <small>班级由管理员统一分配。</small>
      </section>
    </>
  );
}
