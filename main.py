#!/usr/bin/env python3

import datetime
import sys
import commandline
import application


def get_datetime(value):
    if value is None:
        return datetime.datetime.now()
    try:
        return datetime.datetime.fromisoformat(value)
    except ValueError:
        try:
            return datetime.datetime.combine(
                datetime.date.today(), datetime.time.fromisoformat(value)
            )
        except ValueError:
            sys.exit(f"Failed to parse '{value}' into time or datetime.")


if __name__ == "__main__":
    args = commandline.create_parser().parse_args()
    app = application.Application(args.database)
    if args.create_database:
        app.create_database()
    elif args.list_projects:
        print(app.list_projects())
    elif args.add_project:
        app.add_project(args.project)
    elif args.begin:
        app.add_begin(args.project, get_datetime(args.timestamp))
    elif args.end:
        app.add_end(args.project, get_datetime(args.timestamp))
    elif args.list_records:
        app.list_records(args.project)
