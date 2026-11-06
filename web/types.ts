export type Role = 0 | 1 | 2;
export interface User {
  username: string;
  name: string;
  role: Role;
  course: string;
  mustChangePassword: boolean;
}
export interface Grade {
  course: string;
  score: number;
}
export interface Student {
  id: string;
  name: string;
  classId: number;
  teachers: string[];
  grades: Grade[];
}
export interface ClassRecord {
  id: number;
  name: string;
  inactive: boolean;
}
export interface Snapshot {
  students: Student[];
  classes: ClassRecord[];
  users: User[];
}
export interface Result<T> {
  data: T;
  revision: number;
  error?: string;
  csrf?: string;
}
