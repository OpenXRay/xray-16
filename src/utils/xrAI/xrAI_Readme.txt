The following keys are supported / required:
-? or -h       == this help
-f <NAME>      == compile 'level.ai' from 'build.aimap'
    -out <FILE>         == name of output file (default: 'level.ai')
    -draft              == do not load and do not process some stages
    -pure_covers        == <? need investigation ?>
    -keep_temp_files    == do not delete 'build.aimap' after load
-s <NAME,...>  == build game spawn data
    -out <FILE>         == name of output file (default: 'NAME.spawn')
    -start <NAME>       == name of game start level
    -no_separator_check == do not verify that restrictors separates AI map into several disconnected components
-verify <NAME> == verify compiled 'level.ai'
    -noverbose          == do not print all single linked vertices (print only count)

<NAME> == level name as 'gamedata/levels/<NAME>/'
<FILE> == any file name
