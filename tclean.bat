REM cleans the tree
REM (Windows version)
REM $Id$

del   "log"            /s /f /q
rmdir "log"            /s    /q

del   "dumps"          /s /f /q
rmdir "dumps"          /s    /q

del   "*.ram"             /f /q

del   "game1/log"      /s /f /q
rmdir "game1/log"      /s    /q
del   "game1/dumps"    /s /f /q
rmdir "game1/dumps"    /s    /q
del   "game1/*.raw"       /f /q

del   "game2/log"      /s /f /q
rmdir "game2/log"      /s    /q
del   "game2/dumps"    /s /f /q
rmdir "game2/dumps"    /s    /q
del   "game2/*.raw"       /f /q

del   "game3/log"      /s /f /q
rmdir "game3/log"      /s    /q
del   "game3/dumps"    /s /f /q
rmdir "game3/dumps"    /s    /q
del   "game3/*.raw"       /f /q

del   "auth/log"       /s /f /q
rmdir "auth/log"       /s    /q
del   "auth/dumps"     /s /f /q
rmdir "auth/dumps"     /s    /q
del   "auth/*.raw"        /f /q

del   "vault/log"      /s /f /q
rmdir "vault/log"      /s    /q
del   "vault/dumps"    /s /f /q
rmdir "vault/dumps"    /s    /q
del   "vault/*.raw"       /f /q

del   "lobby/log"      /s /f /q
rmdir "lobby/log"      /s    /q
del   "lobby/dumps"    /s /f /q
rmdir "lobby/dumps"    /s    /q
del   "lobby/*.raw"       /f /q

del   "tracking/log"   /s /f /q
rmdir "tracking/log"   /s    /q
del   "tracking/dumps" /s /f /q
rmdir "tracking/dumps" /s    /q
del   "tracking/*.raw"    /f /q

del   "meta/log"       /s /f /q
rmdir "meta/log"       /s    /q
del   "meta/dumps"     /s /f /q
rmdir "meta/dumps"     /s    /q
del   "meta/*.raw"        /f /q
