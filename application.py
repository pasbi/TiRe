import datetime
import pathlib
import sqlite3
import typing
import sys

sqlite3.register_adapter(datetime.datetime, lambda dt: dt.isoformat())


def parse_date(value: typing.Union[datetime.datetime, str]):
    if isinstance(value, str):
        return datetime.datetime.fromisoformat(value)
    return value


def prettify(ts: typing.Union[datetime.datetime, datetime.timedelta]):
    if isinstance(ts, datetime.datetime):
        date = ts.date()
        if date == datetime.date.today():
            date = "Today"
        elif date == datetime.date.today() - datetime.timedelta(day=1):
            date = "Yesterday"
        else:
            date = date.isoformat()

        time = ts.time().isoformat("minutes")
        return f"{date} at {time}"
    else:
        hours, seconds = divmod(ts.total_seconds(), 3600)
        minutes = int(seconds / 60)
        return f"{int(hours):02}:{minutes:02}"


class Record:
    def __init__(self, project, begin, end):
        self.project = project
        self.begin = parse_date(begin)
        self.end = parse_date(end)

    def __repr__(self):
        return f"{self.project}: {prettify(self.begin)} -- {prettify(self.end)}"


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
        print(f"Add project {project_name}.")
        self.cursor.execute("INSERT INTO projects ('name') VALUES (?)", (project_name,))
        self.connection.commit()

    def list_projects(self):
        rows = self.cursor.execute("SELECT name FROM projects").fetchall()
        return [cols[0] for cols in rows]

    def add_begin(self, name: str, timestamp: datetime.datetime):
        if (
            self.cursor.execute(
                "SELECT COUNT(*) FROM projects WHERE name=?", (name,)
            ).fetchall()[0][0]
            != 1
        ):
            sys.exit(f"Project '{name}' is unknown.")

        open_records = self.cursor.execute(
            "SELECT project, end FROM records WHERE end is NULL"
        ).fetchall()
        if len(open_records) > 0:
            sys.exit(f"Close the current project {open_records[0][0]} first")

        print(f"Begin project {name} at {timestamp}.")
        self.cursor.execute(
            "INSERT INTO records ('project', 'begin') VALUES (?, ?)",
            (name, timestamp),
        )
        self.connection.commit()

    def add_end(self, name: str, timestamp: datetime.datetime):
        last = self.cursor.execute(
            "SELECT end, project, rowid FROM records WHERE end IS NULL LIMIT 1"
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

        print(f"End project {name} at {timestamp}")
        self.cursor.execute(
            "UPDATE records SET end=? WHERE rowid=?", (timestamp, last[0][2])
        )
        self.connection.commit()

    def list_records(self, name: typing.Optional[str]):
        if name is None:
            records = self.cursor.execute(
                "SELECT project, begin, end FROM records WHERE end IS NULL"
            ).fetchall()
        else:
            records = self.cursor.execute(
                "SELECT begin, end FROM records WHERE project=? ORDER BY begin", (name,)
            ).fetchall()
        print(records)

    def list_day(self, ts: datetime.datetime, accumulate: bool):
        today = datetime.datetime.combine(ts.date(), datetime.time(hour=0, minute=0))
        tomorrow = today + datetime.timedelta(days=1)
        records = [
            Record(*args)
            for args in self.cursor.execute(
                "SELECT project, begin, end FROM records WHERE begin <= ? AND ? <= end",
                (tomorrow, today),
            ).fetchall()
        ]
        if accumulate:
            accumulated = {}
            for record in records:
                delta = min(record.end, tomorrow) - max(record.begin, today)
                if record.project not in accumulated:
                    accumulated[record.project] = datetime.timedelta()
                accumulated[record.project] += delta

            for project, time_spent in accumulated.items():
                print(f"{project}: {prettify(time_spent)}")

        else:
            print(records)
