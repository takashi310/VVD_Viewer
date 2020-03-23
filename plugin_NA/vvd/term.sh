#!/bin/sh

echo '
on run argv
  if length of argv is equal to 0
    set command to ""
  else
    set command to item 1 of argv
  end if

  if application "Terminal" is running then
    set terQuit to false
  else
    set terQuit to true
  end if

  tell application "Terminal"
    if terQuit then
      activate
      set currentTab to do script(command) in window 1
      set frontWindow to currentTab
      delay 1
      activate
      delay 2
      repeat until busy of frontWindow is false
        delay 1
      end repeat
      do script("kill -9 $PPID") in currentTab
      quit
    else
      activate
      set newTab to do script(command)
      set frontWindow to newTab
      delay 3
      repeat until busy of frontWindow is false
        delay 1
      end repeat
      do script("kill -9 $PPID") in newTab
    end if
  end tell

end run

' | osascript - "$@"
