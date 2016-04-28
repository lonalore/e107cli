<?php

/**
 * @file
 * Add-on file to describe e107cli core commands.
 */


/**
 * Class core_e107cli.
 *
 * Using add-on file in e107 plugins:
 *   @code
 *     class PLUGIN_e107cli() {

 *     }
 *   @endcode
 *
 * Using add-on file in e107 themes:
 *   @code
 *     class THEME_e107cli() {

 *     }
 *   @endcode
 */
class core_e107cli {

  /**
   * In this method, you specify which commands your e107cli plugin makes
   * available, what it does and description.
   *
   * @return array $commands
   *   An associative array describing your command(s).
   */
  function config() {
    $commands = array();

    $commands['help'] = array(
      'bootstrap' => E107CLI_BOOTSTRAP_BASE, // No bootstrap e107.
      'description' => 'Print this help message.',
      'examples' => array(
        'e107cli help' => 'A list of available commands, one per line.',
      ),
      'callback' => 'e107cli_core_help',
      'file' => E107CLI_BASE_PATH . '/commands/core.help.inc',
      'arguments' => array(),
      'options' => array(),
    );

    return $commands;
  }

}
