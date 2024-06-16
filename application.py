import datetime
import itertools
import pathlib
import sqlite3
import typing
import sys

sqlite3.register_adapter(datetime.datetime, lambda dt: dt.isoformat())


class Application:
    def __init__(self, filename: pathlib.Path):
        self.connection = sqlite3.connect(filename)
        self.cursor = self.connection.cursor()

    def create_database(self):
        self.cursor.execute(
            """
            CREATE TABLE projects(
                name VARCHAR(255) NOT NULL,
                creation_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL,
                PRIMARY KEY(name)
            )
            """
        )
        self.cursor.execute(
            """
            CREATE TABLE records(
                project VARCHAR(255) NOT NULL,
                creation_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL,
                begin DATETIME NOT NULL,
                end DATETIME,
                FOREIGN KEY(project) REFERENCES projects(name)
            )
            """
        )

    def add_project(self, project_name: str):
        self.cursor.execute("INSERT INTO projects ('name') VALUES (?)", (project_name,))
        self.connection.commit()

    def list_projects(self):
        rows = self.cursor.execute("SELECT name FROM projects").fetchall()
        return [cols[0] for cols in rows]

    def add_begin(self, name: str, timestamp: datetime.datetime):
        last = self.cursor.execute(
            "SELECT end, project FROM records ORDER BY begin DESC LIMIT 1"
        ).fetchall()

        if len(last) > 0 and last[0][0] is None:
            sys.exit(f"Close the current project {last[0][1]} first")

        self.cursor.execute(
            "INSERT INTO records ('project', 'begin') VALUES (?, ?)",
            (name, timestamp),
        )
        self.connection.commit()

    def add_end(self, name: str, timestamp: datetime.datetime):
        last = self.cursor.execute(
            "SELECT end, project, rowid FROM records ORDER BY begin DESC LIMIT 1"
        ).fetchall()

        if len(last) == 0 or last[0][0] is not None:
            sys.exit(f"There is no project to close.")

        expected_project_name = last[0][1]
        if name is None:
            name = expected_project_name
        elif name != expected_project_name:
            sys.exit(
                f"Provided project name '{name}' does not match open project '{expected_project_name}'. "
                "Tip: don't provide a project name."
            )
        self.cursor.execute(
            "UPDATE records SET end=? WHERE rowid=?", (timestamp, last[0][2])
        )
        self.connection.commit()

    def list_records(self, name: typing.Optional[str]):
        if name is None:
            records = self.cursor.execute(
                "SELECT project, begin, end from records ORDER BY begin"
            ).fetchall()
        else:
            records = self.cursor.execute(
                "SELECT begin, end from records WHERE project=? ORDER BY begin", (name,)
            ).fetchall()

        print(records)
