#!/usr/bin/env php
<?php

/**
 * @file
 * e107cli is a PHP script implementing a command line shell for e107.
 *
 * @requires PHP CLI 5.3.0, or newer.
 */


require(dirname(__FILE__) . '/includes/bootstrap.inc');

if (e107cli_bootstrap_prepare() === FALSE) {
  exit(1);
}

exit(e107cli_main());


/**
 * The main e107cli function.
 *
 * @return mixed
 *   Whatever the given command returns.
 */
function e107cli_main() {
  // Process initial global options such as --debug.
  _e107cli_bootstrap_global_options();

  print 'Working directory: ';
  print "\n";
  print_r(e107cli_cwd());
  print "\n";
  print "\n";

  print 'Commands: ';
  print "\n";
  print_r(e107cli_get_arguments());
  print "\n";

  print 'Command arguments: ';
  print "\n";
  print_r(e107cli_get_context('E107CLI_COMMAND_ARGS'));
  print "\n";

  print 'Command options: ';
  print "\n";
  print_r(e107cli_get_context('E107CLI_COMMAND_OPTIONS'));
  print "\n";
}
