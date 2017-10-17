# unitscript

This program can be used as an interpreter for unitscripts, which are yaml based init scripts.
Unitscripts could be used as easier alternative to shell scripts as init scripts or to standardise init scripts
across many different sysv-rc compatible distributions. This project was created to show how to create something
similar to systemd units which could be used to standardise init scripts across distributions but using
existing technologies and without imposing a system which has since grown way bayond any reasonable scope on everybody.

This project does only provide some basic functionality to start and stop programs at the moment and may be extendet
by additional features in the future. It will, however, never become a daemon in the classical sense and unitscripts
will never do anything else than managing services.

## Currently supported actions

| action | description |
|-----------|-------------|
| start | Starts the program. The script won't exit until the 'start check' condition is met. |
| stop | Stops the program by sending it a SIGTERM signal. Won't exit until the process whose pid is written in the pid file doesn't exist anymore or an error occurs. |
| restart | shorthand for start and stop actions. |
| status | Returns if the process is running and on which pid or not. Exit code will be either 0 for yes, 1 fo no, or anything else to indicate an error. |
| check | Does nothing, but will still exit with an error message if the unitscript contains an error |
| zap | Removes the pid file |

## Format

A unitscript consists of the shebang line ```#!/bin/unitscript```, followed by an LSB Header,
followed by the unitscript options. The wholeunit script is a valid yaml file.

The following unitscript options are currently supported:

| option      | type    | description |
|-------------|---------|-------------|
| user        | string  | A valid user name from /etc/passwd |
| group       | string  | A valid group name from /etc/group |
| start check | string  | Can be either exit, start, or notification. <br/> exit: The process forks itself, wait until it exits and check the return code. <br/> start: The moment the executable is executed is considered a successful start. <br/> notification: The process writes a newline to a file descriptor to indicate the start succeeded. You can specify the file descriptor using the notifyfd property, default is 3. |
| program     | string  | A shell script. The default interpreter is ```sh -l```, but can be overwritten using a shebang line |
| uid         | integer | A user id of a valid user from /etc/passwd |
| gid         | integer | A group id of a valid group from /etc/group |
| notifyfd    | integer | Used if ```start check``` is set to notification. Default is 3. |
| logging     | string  | If and how stdout and stdin should be redirected. Can be either: default, syslog, stdio or none. <br/> default: equivalent to syslog if 'start check' is 'start', stdout if 'start check' is 'exit', invalid otherwise. <br/> syslog: redirect stdout and stderr to syslog before executing 'program'. <br/> stdio: keep stdout and stderr unchanged <br/> none: close stdout and stderr before execution. |
| pidfile     | string  | Defaults to /var/run/unitscript/<script name>.pid |
| manage pidfile | boolean | Either yes or no. Specifies if the program or the unit script should create the pid file. <br/> Default for 'start check' 'exit' is 'no'. <br/> Default for 'start check' 'start' is 'yes'. |
| env         | map     | Environment variables to set before program execution |
| working directory | string | Sets the working directory. Defaults to the user home directory. |
| umask       | integer | The umask using which the program is started, defaults to 0022 |


## Environment variables

All environment variables will be cleared before execution, but per default a sh login shell is used to start the program,
which should source /etc/profile and .profile, which may set some basic environment variables like the PATH variable.
In addition to this, all variables specified using the 'env' option and the following ones will be set:

| variable | description |
|----------|-------------|
| SHELL    | The shell specified for the user in /etc/passwd |
| HOME     | The user home directory as specified in /etc/passwd |
| PIDFILE  | The location of the pid file |
| NOTIFICATION_FD | The file descriptor number using which the program can indicate that it finished starting up |
