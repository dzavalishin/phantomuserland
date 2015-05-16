## Implemented ##

None

## Proposed data types ##

  * ShellItem - preview(), view(), open(), print()
  * SystemTray - add/remove TrayItem
  * SearchAgent - search(name, tags), getPossibleTags()?


## Proposed calls ##
  * ` GetInbox( user ).post( objectToPassToUser ) - object is to appear in user's shell inbox `
  * ` openShellWindow( List<Tags> filterBy, List<Tags> sortBy ) `
  * ` getSystemTray() `
  * ` registerSearchAgent( SearchAgent ) - agent will be called on global search to search in app's data. `