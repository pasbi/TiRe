import argparse


def create_parser():
    parser = argparse.ArgumentParser(
        prog="tire", description="Simple Time Recording tool."
    )
    parser.add_argument(
        "--database", help="The database file.", required=True, metavar="FILENAME"
    )
    parser.add_argument(
        "--timestamp",
        help="The timestamp of the begin or end. Default to $now. If only a time is given, assumes $today at that time.",
        default=None,
    )
    parser.add_argument("--project", help="The project name.")

    command_group = parser.add_mutually_exclusive_group()
    command_group.add_argument(
        "--add-project", help="Add a project.", action="store_true", default=False
    )
    command_group.add_argument(
        "--begin",
        help="Start working on a project.",
        action="store_true",
        default=False,
    )
    command_group.add_argument(
        "--end",
        help="Stop working on a project.",
        action="store_true",
        default=False,
    )
    command_group.add_argument(
        "--list-records", help="List all records.", action="store_true", default=False
    )
    command_group.add_argument(
        "--list-projects", help="List all projects.", action="store_true", default=False
    )
    command_group.add_argument(
        "--create-database",
        help="Creates a new database.",
        action="store_true",
        default=False,
    )
    return parser
