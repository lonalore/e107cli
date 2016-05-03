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
  $global_options = array();
  $target_name = NULL;

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
      // If we've found the command name, then save $options in $global_options
      // (all options that came before the command name), and initialize
      // $command_args to an array so that we will begin storing all args
      // and options that follow the command name in $command_args.
      if (empty($arguments)) {
        $command_args = array();
        $global_options = $options;
        $options = array();
      }

      $arguments[] = $opt;
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

  e107cli_set_context('E107CLI_GLOBAL_OPTIONS', $global_options);
  e107cli_set_arguments($arguments);
  e107cli_set_context('E107CLI_COMMAND_OPTIONS', $options);

  return $arguments;
}

/**
 * Matches a commands array, as returned by e107cli_get_arguments, with the
 * current command table.
 */
function e107cli_parse_command() {
  $args = e107cli_get_arguments();
  $command = FALSE;

  // Get a list of all implemented commands.
  $implemented = e107cli_get_commands();
  if (!empty($args) && isset($implemented[$args[0]])) {
    $command = $implemented[$args[0]];
    $arguments = array_slice($args, 1);
    _e107cli_prepare_command($command, $arguments);
  }

  // We have found a command that matches. Set the appropriate values.
  if ($command) {
    // Special case. Force help command if --help, -h options were specified.
    if (e107cli_get_option('help') || e107cli_get_option('h')) {
      $arguments = array($command['command']);
      $command = $implemented['help'];
      _e107cli_prepare_command($command, $arguments);
    }
    else {
      _e107cli_prepare_command($command, $arguments);
    }

    e107cli_set_command($command);
  }

  return $command;
}

/**
 * Called by e107cli_parse_command.
 */
function _e107cli_prepare_command(&$command, $arguments = array()) {
  // E107cli overloads $command['arguments']; save the argument description.
  if (!isset($command['argument-description'])) {
    $command['argument-description'] = $command['arguments'];
  }

  $command['arguments'] = $arguments;
}

/**
 * Collect a list of all available e107cli command files.
 *
 * Scans the following paths for e107cli command files:
 *
 * - Core e_e107cli.php
 * - e_e107cli.php files belonging to enabled modules in the current e107 site.
 * - e_e107cli.php files belonging to enabled themes in the current e107 site.
 *
 * @return
 *   An associative array whose keys and values are the names of all available
 *   command files.
 */
function e107cli_commandfile_list() {
  return e107cli_get_context('E107CLI_COMMAND_FILES', array());
}

function _e107cli_find_commandfiles($phase) {
  $searchpath = array();

  switch ($phase) {
    case E107CLI_BOOTSTRAP_BASE:
      $searchpath['core'] = E107CLI_BASE_PATH . '/e_e107cli.php';
      break;

    case E107CLI_BOOTSTRAP_E107:
      // TODO: scan e107 directories to find command files.
      break;
  }

  if (count($searchpath)) {
    $commandfiles = e107cli_get_context('E107CLI_COMMAND_FILES', array());

    foreach ($searchpath as $prefix => $path) {
      if (!isset($commandfiles[$prefix])) {
        $commandfiles[$prefix] = $path;
      }
    }

    e107cli_set_context('E107CLI_COMMAND_FILES', $commandfiles);
  }
}

/**
 * Get a list of all implemented commands.
 *
 * @return array
 *   Associative array of currently active command descriptors.
 */
function e107cli_get_commands() {
  $commands = array();

  $list = e107cli_commandfile_list();
  foreach ($list as $prefix => $path) {
    if (file_exists($path)) {
      // Include add-on file.
      require_once $path;

      $class = $prefix . '_e107cli';

      if (!class_exists($class)) {
        continue;
      }

      $addon = new $class();

      if (!method_exists($addon, 'config')) {
        continue;
      }

      $commands = $addon->config();

      foreach ($commands as $key => $command) {
        $command += e107cli_command_defaults($key, $command);

        if (e107cli_get_context('E107CLI_BOOTSTRAP_PHASE', E107CLI_BOOTSTRAP_BASE) >= $command['bootstrap']) {
          $commands[$key] = $command;

          // For every alias, make a copy of the command and store it in the
          // command list using the alias as a key.
          if (isset($command['aliases']) && count($command['aliases'])) {
            foreach ($command['aliases'] as $alias) {
              $commands[$alias] = $command;
              $commands[$alias]['is_alias'] = TRUE;
            }
          }
        }
      }
    }
  }

  return $commands;
}

function e107cli_command_defaults($key, $command) {
  return array(
    'command' => $key,
    'bootstrap' => E107CLI_BOOTSTRAP_BASE,
    'description' => '',
    'aliases' => array(),
    'arguments' => array(),
    'options' => array(),
    'examples' => array(),
    'hidden' => FALSE,
    'callback' => '',
    'file' => '',
    'is_alias' => FALSE,
  );
}

/**
 * Given a command record, dispatch it as if it were the original command.
 * Executes in the currently bootstrapped site using the current option
 * contexts.
 * Note that e107cli_dispatch will not bootstrap any further than the current
 * command has already bootstrapped; therefore, you should only invoke commands
 * that have the same (or lower) bootstrap requirements.
 *
 * @param array $command
 *   A full $command such as returned by e107cli_get_commands(), or a string
 *   containing the name of the command record from e107cli_get_commands() to
 *   call.
 * @param array $arguments
 *   An array of argument values.
 *
 * @return boolean
 */
function e107cli_dispatch($command) {
  e107cli_set_command($command);
  $return = FALSE;

  if ($command) {
    $arguments = e107cli_get_context('E107CLI_COMMAND_ARGS', array());
    $options = e107cli_get_context('E107CLI_COMMAND_OPTIONS', array());

    // Add arguments, if this has not already been done.
    // (If the command was fetched from e107cli_parse_command, then you cannot
    // provide arguments to e107cli_dispatch.)
    if (empty($command['arguments'])) {
      _e107cli_prepare_command($command, $arguments);
    }

    // Include and validate command engines.
    if (isset($command['file'])) {
      if (!file_exists($command['file'])) {
        return FALSE;
      }

      require_once $command['file'];
    }

    // Call the callback function of the active command.
    $return = call_user_func_array($command['callback'], array(
      $options,
      $arguments
    ));
  }

  return $return;
}
