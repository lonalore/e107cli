<?php

/**
 * @file
 * The e107cli context API implementation
 */


/**
 * Set a specific context.
 *
 * @param context
 *   Any of the default defined contexts.
 * @param value
 *   The value to store in the context
 *
 * @return
 *   An associative array of the settings specified in the request context.
 */
function e107cli_set_context($context, $value) {
  $cache = &e107cli_get_context($context);
  $cache = $value;
  return $value;
}

/**
 * Return a specific context, or the whole context cache.
 *
 * This function provides a storage mechanism for any information the currently
 * running process might need to communicate.
 *
 * This avoids the use of globals, and constants.
 *
 * Functions that operate on the context cache, can retrieve a reference to the
 * context cache using:
 *
 * $cache = &e107cli_get_context($context);
 *
 * This is a private function, because it is meant as an internal generalized
 * API for writing static cache functions, not as a general purpose function to
 * be used inside commands.
 *
 * Code that modifies the reference directly might have unexpected consequences,
 * such as modifying the arguments after they have already been parsed and
 * dispatched to the callbacks.
 *
 * @param context
 *   Optional. Any of the default defined contexts.
 *
 * @return
 *   If context is not supplied, the entire context cache will be returned.
 *   Otherwise only the requested context will be returned.
 *   If the context does not exist yet, it will be initialized to an empty array.
 */
function &e107cli_get_context($context = NULL, $default = NULL) {
  static $cache = array();
  if (isset($context)) {
    if (!isset($cache[$context])) {
      $default = !isset($default) ? array() : $default;
      $cache[$context] = $default;
    }
    return $cache[$context];
  }
  return $cache;
}

/**
 * Get the value for an option.
 *
 * If the first argument is an array, then it checks whether one of the options
 * exists and return the value of the first one found. Useful for allowing both
 * -h and --host-name
 *
 * @param option
 *   The name of the option to get.
 * @param default
 *   Optional. The value to return if the option has not been set.
 * @param context
 *   Optional. The context to check for the option. If this is set, only this
 *   context will be searched.
 *
 * @return
 */
function e107cli_get_option($option, $default = NULL, $context = NULL) {
  if ($context != NULL) {
    if (is_array($option)) {
      foreach ($option as $current) {
        if (array_key_exists($current, $context)) {
          return $context[$current];
        }
      }
    }
    elseif (array_key_exists($option, $context)) {
      return $context[$option];
    }
  }
  else {
    $contexts = e107cli_get_context();

    foreach ($contexts as $context) {
      if (is_array($option)) {
        foreach ($option as $current) {
          if (array_key_exists($current, $context)) {
            return $context[$current];
          }
        }
      }
      elseif (is_array($context) && array_key_exists($option, $context)) {
        return $context[$option];
      }
    }
  }

  return $default;
}

/**
 * Set an option in one of the option contexts.
 *
 * @param option
 *   The option to set.
 * @param value
 *   The value to set it to.
 * @param string $context
 *   Optional. Which context to set it in.
 * @return
 *   The value parameter. This allows for neater code such as
 *   $myvalue = e107cli_set_option('http_host', $_SERVER['HTTP_HOST']);
 *   Without having to constantly type out the value parameter.
 */
function e107cli_set_option($option, $value, $context = 'process') {
  $cache = &e107cli_get_context($context);
  $cache[$option] = $value;
  return $value;
}

/**
 * Remove a setting from a specific context.
 *
 * @param
 *   Option to be unset
 * @param
 *   Context in which to unset the value in.
 */
function e107cli_unset_option($option, $context = NULL) {
  if ($context != NULL) {
    $cache = &e107cli_get_context($context);
    if (array_key_exists($option, $cache)) {
      unset($cache[$option]);
    }
  }
  else {
    $contexts = e107cli_get_context();

    foreach ($contexts as $context) {
      e107cli_unset_option($option, $context);
    }
  }
}

/**
 * Set the arguments passed to the e107cli.php script.
 *
 * This function will set the 'arguments' context of the current running script.
 *
 * When initially called by e107cli_parse_args, the entire list of arguments
 * will be populated. Once the command is dispatched, this will be set to only
 * the remaining arguments to the command (i.e. the command name is removed).
 *
 * @param arguments
 *   Command line arguments, as an array.
 */
function e107cli_set_arguments($arguments) {
  e107cli_set_context('arguments', $arguments);
}

/**
 * Gets the command line arguments passed to e107cli.
 *
 * @return array
 *   An indexed array of arguments. Until e107cli has dispatched the command,
 *   the array will include the command name as the first element. After that
 *   point the array will not include the command name.
 *
 * @see e107cli_set_arguments()
 */
function e107cli_get_arguments() {
  return e107cli_get_context('arguments');
}

/**
 * Set the command being executed.
 *
 * e107cli_dispatch will set the correct command based on it's matching of the
 * script arguments retrieved from e107cli_get_arguments to the implemented
 * commands specified by e107cli_get_commands.
 *
 * @param
 *   A numerically indexed array of command components.
 */
function e107cli_set_command($command) {
  e107cli_set_context('command', $command);
}

/**
 * Return the command being executed.
 */
function e107cli_get_command() {
  return e107cli_get_context('command');
}
