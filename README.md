# Time Recording

I was tired of tracking my times using pen and paper of buggy excel sheets.
This is a very simplistic python wrapper around a sqlite database storing all the time records.

```
# Create a new database
tire/main.py --database tracking.db --create-database

# Add a project
tire/main.py --database tracking.db --project ProjectA --add-project

# Begin
tire/main.py --database tracking.db --project ProjectA --begin

# Actually work on ProjectA for 3h and 45min time ...

# End
tire/main.py --database tracking.db --project ProjectA --end

# List
tire/main.py --database tracking.db --list-day --accumulate
ProjectA: 3:45
```

## Features:

- Simple command line interface.
- Sensible defaults: most commands accept a `--timestamp` option but default to today or now if omitted.
- Uses a mature database for storing the records.
- Set-up within a second.
- Works local without internet connection or fees.
- Easily extendable.
