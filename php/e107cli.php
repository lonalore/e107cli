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

  // Try to bootstrap.
  e107cli_bootstrap_to_phase(E107CLI_BOOTSTRAP_BASE);

  $return = '';
  if (!e107cli_get_error()) {
    // Try to bootstrap to max.
    e107cli_bootstrap_to_phase(E107CLI_BOOTSTRAP_E107);

    // Get the command.
    $command = e107cli_parse_command();
    if (is_array($command)) {
      $return = _e107cli_bootstrap_and_dispatch();
    }

    if ($command === FALSE) {
      // Set errors related to this command.
      $args = implode(' ', e107cli_get_arguments());

      $error_msg = "The e107cli command '" . $args . "' could not be found.";
      e107cli_set_error('E107CLI_COMMAND_NOT_FOUND', $error_msg);
      e107cli_print($error_msg);
    }
  }
  e107cli_bootstrap_finish();

  // After this point the e107cli_shutdown function will run, exiting with the
  // correct exit code.
  return $return;
}

function _e107cli_bootstrap_and_dispatch() {
  $return = '';
  $command_found = FALSE;

  $command = e107cli_get_command();
  if (is_array($command)) {
    e107cli_bootstrap_to_phase($command['bootstrap']);

    if (e107cli_get_context('E107CLI_BOOTSTRAP_PHASE', E107CLI_BOOTSTRAP_BASE) >= $command['bootstrap']) {
      $command_found = TRUE;
      // Dispatch the command(s).
      $return = e107cli_dispatch($command);
      // Prevent a '1' at the end of the output.
      if ($return === TRUE) {
        $return = '';
      }
    }
  }

  if (!$command_found) {
    // Set errors related to this command.
    $args = implode(' ', e107cli_get_arguments());

    if (isset($command) && is_array($command)) {
      $error_msg = "The e107cli command '" . $args . "' could not be executed.";
      e107cli_set_error('E107CLI_COMMAND_NOT_EXECUTABLE', $error_msg);
      e107cli_print($error_msg);
    }
    elseif (!empty($args)) {
      $error_msg = "The e107cli command '" . $args . "' could not be found.";
      e107cli_set_error('E107CLI_COMMAND_NOT_FOUND', $error_msg);
      e107cli_print($error_msg);
    }

    // Set errors that occurred in the bootstrap phases.
    $errors = e107cli_get_context('E107CLI_BOOTSTRAP_ERRORS', array());
    foreach ($errors as $code => $message) {
      e107cli_set_error($code, $message);
    }
  }

  return $return;
}
