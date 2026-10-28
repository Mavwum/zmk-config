"""Keep Studio cancellation safe when a response has already completed.

The pinned Zephyr lookup inspects idle ATT channels without checking req.
Use its existing scheduler-locked cancellation path, with the missing guard.
Upstream/default firmware sources stay untouched.
"""
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
old = "if (chan->req->user_data == user_data) {"
if source.count(old) != 1:
    raise SystemExit("Pinned ATT request lookup changed")
source = source.replace(old, "if (chan->req && chan->req->user_data == user_data) {", 1)
Path(sys.argv[2]).write_text(source)
