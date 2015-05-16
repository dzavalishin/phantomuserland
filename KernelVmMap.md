# General idea #

VM subsystem has two global states: normal operation and snapshot creation. Normal operation supposed to be just like anyh other virtual memory subsystem. Snapshot creation is described below.

## Disk relations ##

  * Not backed by mem or disk at all. Read as null, any access allocates memory. (Disk too?)
  * Backed by disk - up to three disk blocks. Between snaps prev page is page from last good snap we have. We CAN NOT write to this page never ever. If page is to be written, it goes to curr page ONLY. Make page is a disk page which has copy which will go to new snapshot. It can be the same as curr/prev if page is not changed before/during snap. If page was changed before snap, curr will differ from prev. If page is changed during snap, make will differ from curr (andf curr will have new, more recent state).

## Snapshot ##

Snapshot task is to guarantee that all pages have make\_page and it contains correct data.

  * Try to pageout as much as possible

  * Turn on write protection on all pages.

  * If page is written - COW (old data to make\_page, new to curr)

  * Now run through all pages and check they have make\_page - if not - touch 'em. (Is it right? It causes excess disk usage?)