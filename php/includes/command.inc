<?php

/**
 * @file
 * The e107cli command engine.
 */


/**
 * Parse console arguments.
 */
function e107cli_parse_args() {
  $args = e107cli_get_context('argv');
  $command_args = NULL;
  $target_alias_name = NULL;

  // It would be nice if commandfiles could somehow extend this list,
  // but it is not possible. We need to parse args before we find commandfiles,
  // because the specified options may affect how commandfiles are located.
  // Therefore, commandfiles are loaded too late to affect arg parsing.
  // There are only a limited number of short options anyway; e107cli reserves
  // all for use by e107cli core.
  static $arg_opts = array('c', 'u', 'r', 'l', 'i');

  // Now process the command line arguments. We will divide them into options
  // (starting with a '-') and arguments.
  $arguments = $options = array();
  for ($i = 1; $i < count($args); $i++) {
    $opt = $args[$i];

    // We set $command_args to NULL until the first argument that is not an
    // alias is found (the command); we put everything that follows into
    // $command_args.
    if (is_array($command_args)) {
      $command_args[] = $opt;
    }

    // Is the arg an option (starting with '-')?
    if (!empty($opt) && $opt{0} == "-" && strlen($opt) != 1) {
      // Do we have multiple options behind one '-'?
      if (strlen($opt) > 2 && $opt{1} != "-") {
        // Each char becomes a key of its own.
        for ($j = 1; $j < strlen($opt); $j++) {
          $options[substr($opt, $j, 1)] = TRUE;
        }
      }
      // Do we have a longopt (starting with '--')?
      elseif ($opt{1} == "-") {
        if ($pos = strpos($opt, '=')) {
          $options[substr($opt, 2, $pos - 2)] = substr($opt, $pos + 1);
        }
        else {
          $options[substr($opt, 2)] = TRUE;
        }
      }
      else {
        $opt = substr($opt, 1);
        // Check if the current opt is in $arg_opts (= has to be followed by an
        // argument).
        if ((in_array($opt, $arg_opts))) {
          // Raising errors for missing option values should be handled by the
          // bootstrap or specific command, so we no longer do this here.
          $options[$opt] = $args[$i + 1];
          $i++;
        }
        else {
          $options[$opt] = TRUE;
        }
      }
    }
    // If it's not an option, it's a command.
    else {
      $arguments[] = $opt;
      // Once we find the first argument, record the command args and global
      // options.
      if (!is_array($command_args)) {
        $command_args = array();
      }
    }
  }

  // If no arguments are specified, then the command will be either 'help' or
  // 'version' (the later if --version is specified)
  if (!count($arguments)) {
    if (array_key_exists('version', $options)) {
      $arguments = array('version');
    }
    else {
      $arguments = array('help');
    }
  }

  if (is_array($command_args)) {
    e107cli_set_context('E107CLI_COMMAND_ARGS', $command_args);
  }

  e107cli_set_arguments($arguments);
  e107cli_set_context('E107CLI_COMMAND_OPTIONS', $options);

  return $arguments;
}

/**
 * Get a list of all implemented commands.
 *
 * @return array
 *   Associative array of currently active command descriptors.
 */
function e107cli_get_commands() {

}
