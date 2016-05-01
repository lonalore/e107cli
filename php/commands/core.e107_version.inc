<?php

/**
 * @file
 * Callback and helper functions for command 'e107-version'.
 */


/**
 * Callback function for command 'e107-version'.
 */
function e107cli_core_e107_version($options = array(), $args = array()) {
  return e_VERSION;
}
