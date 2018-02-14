# Concurrent_Text_Search
This program uses multiple processes combined with IPC to search for text across multiple files.
The main process spawns as many child processes as there are files to search from.
Each child process will search for a given text within a particular file and
report the result back to the parent. After all the children send their search result,
the parent then summarizes the search results.
