# e107cli

E107cli is a command line shell and Unix scripting interface for e107 v2.

## USAGE

E107cli can be run in your shell by typing "e107cli" from within any e107 (v2) root directory.

    $ e107cli <command> [argument1] [argument2]

Use the 'help' command to get a list of available options and commands:

    $ e107cli help

## REQUIREMENTS

* GIT 1.7 or greater.
* Composer - Dependency Management
* E107cli works on a Unix-like OS (Linux, OS X)

## INSTALL - MANUAL

* Clone this repository `git clone https://github.com/lonalore/e107cli.git` in a directory that is outside of your web root.
* Run `composer install` to fetch dependencies.
* Make the 'e107cli' command executable: `$ chmod u+x /path/to/e107cli/bin/e107cli`
* Create a symbolic link to the 'e107cli' executable in a directory that is already in your PATH, e.g.: `$ ln -s /path/to/e107cli/bin/e107cli /usr/bin/e107cli`
* Test that e107cli is found by your system: `$ which e107cli`

