<?php

/**
 * @file
 * Callback and helper functions for command 'help'.
 */


/**
 * This is called if no command or an unknown command is entered.
 */
function e107cli_core_help($options = array(), $arguments = array()) {
  // e107cli help
  if (empty($arguments)) {
    e107cli_core_help_show_commands();
  }
  // e107cli help -h
  // e107cli help --help
  else {
    $implemented = e107cli_get_commands();

  }
}

function e107cli_core_help_show_commands() {
  $phases = _e107cli_bootstrap_phases();
  // For speed, only bootstrap up to E107CLI_BOOTSTRAP_E107+1.
  $phases = array_slice($phases, 0, E107CLI_BOOTSTRAP_E107 + 1);

  e107cli_print('Commands: ');

  $printed_rows = array();
  $phase_index = E107CLI_BOOTSTRAP_BASE;

  foreach ($phases as $phase_index) {
    if (e107cli_bootstrap_validate($phase_index)) {
      if ($phase_index > e107cli_get_context('E107CLI_BOOTSTRAP_PHASE', E107CLI_BOOTSTRAP_BASE)) {
        e107cli_bootstrap($phase_index);
      }

      $commands = e107cli_get_commands();

      // Filter by command file if specified.
      if ($commandfile = e107cli_get_option('filter')) {
        foreach ($commands as $key => $candidate) {
          if ($candidate['commandfile'] != $commandfile) {
            unset($commands[$key]);
          }
        }
      }

      $rows = array();

      ksort($commands);
      foreach ($commands as $key => $command) {
        if (!$command['hidden']) {
          if (!array_key_exists('is_alias', $command) || !$command['is_alias']) {
            if (!array_key_exists($key, $printed_rows)) {
              $name = $command['aliases'] ? $key . ' (' . implode(', ', $command['aliases']) . ')' : $key;
              $rows[$key] = array($name, $command['description']);
              $pipe[] = "\"$key\"";
            }
          }
        }
      }

      e107cli_print_table($rows, FALSE, array(0 => 20));
      $printed_rows = array_merge($printed_rows, $rows);
    }
    else {
      break;
    }
  }

  if (isset($pipe)) {
    // Newline-delimited list for use by other scripts. Set the --pipe option.
    e107cli_print_pipe($pipe);
    return;
  }
}
