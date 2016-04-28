<?php

/**
 * @file
 * Callback and helper functions for command 'version'.
 */


/**
 * Callback function for command 'version'.
 */
function e107cli_core_version($options = array(), $args = array()) {
  if (array_key_exists('major', $options)) {
    return E107CLI_MAJOR_VERSION . PHP_EOL;
  }
  elseif (array_key_exists('minor', $options)) {
    return E107CLI_MINOR_VERSION . PHP_EOL;
  }
  else {
    return E107CLI_VERSION . PHP_EOL;
  }
}
