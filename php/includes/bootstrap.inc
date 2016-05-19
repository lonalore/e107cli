<?php

/**
 * @file
 *
 * E107cli bootstrapping code.
 *
 * Functions here are used to bootstrap e107cli so that we can run commands.
 * This is also where third party extensions can bootstrap e107cli without going
 * through e107cli.php (this is used in the unit tests for example).
 */


/**
 * @name E107cli bootstrap phases
 * @{
 * Sequential e107cli bootstrapping phases.
 */

/**
 * Only bootstrap e107cli, without any e107 specific code.
 *
 * Any code that operates on the e107cli installation, and not specifically
 * any e107 directory, should bootstrap to this phase.
 */
define('E107CLI_BOOTSTRAP_BASE', 0);

/**
 * Set up and test for a valid e107 root, either through the -r/--root options,
 * or evaluated based on the current working directory.
 *
 * Any code that interacts with an entire e107 installation should use this
 * bootstrap phase.
 */
define('E107CLI_BOOTSTRAP_E107', 1);

/**
 * Helper function listing phases.
 *
 * For commands that need to iterate through the phases, such as help.
 */
function _e107cli_bootstrap_phases($function_names = FALSE, $init_phases_only = FALSE) {
  static $functions = array(
    E107CLI_BOOTSTRAP_BASE => '_e107cli_bootstrap_base',
    E107CLI_BOOTSTRAP_E107 => '_e107cli_bootstrap_e107',
  );

  $result = array();

  if ($init_phases_only) {
    foreach (array(E107CLI_BOOTSTRAP_BASE, E107CLI_BOOTSTRAP_E107) as $phase) {
      $result[$phase] = $functions[$phase];
    }
  }
  else {
    $result = $functions;
  }

  if (!$function_names) {
    $result = array_keys($result);
  }

  return $result;
}

/**
 * @} End of E107cli bootstrap phases.
 */

/**
 * Prepare e107cli for bootstrap.
 *
 * All pre-flight checks and includes necessary to begin the bootstrap process.
 *
 * Ran before e107cli_main().
 *
 * @see e107cli_main()
 * @see e107cli.php
 */
function e107cli_bootstrap_prepare() {
  define('E107CLI_BASE_PATH', dirname(dirname(__FILE__)));

  // Local means that autoload.php is inside of e107cli. That is, e107cli is
  // its own Composer project. Global means autoload.php is outside of e107cli.
  // That is, e107cli is a dependency of a bigger project.
  $local_vendor_path = E107CLI_BASE_PATH . '/../vendor/autoload.php';
  $global_vendor_path = E107CLI_BASE_PATH . '/../../../../vendor/autoload.php';

  // Check for a local composer install or a global composer install. Vendor
  // dirs are in different spots).
  if (file_exists($local_vendor_path)) {
    $vendor_path = $local_vendor_path;
  }
  elseif (file_exists($global_vendor_path)) {
    $vendor_path = $global_vendor_path;
  }
  else {
    $msg = "Unable to load autoload.php. Run 'composer install' to fetch dependencies.\n";
    fwrite(STDERR, $msg);
    return FALSE;
  }

  $classloader = require $vendor_path;

  require_once E107CLI_BASE_PATH . '/includes/environment.inc';
  require_once E107CLI_BASE_PATH . '/includes/e107cli.inc';
  require_once E107CLI_BASE_PATH . '/includes/context.inc';
  require_once E107CLI_BASE_PATH . '/includes/command.inc';

  // Stash our vendor path and classloader.
  e107cli_set_context('E107CLI_VENDOR_PATH', dirname($vendor_path));
  e107cli_set_context('E107CLI_CLASSLOADER', $classloader);

  // Terminate immediately unless invoked as a command line script.
  if (!e107cli_verify_cli()) {
    return e107cli_error('E107cli is designed to run via the command line.');
  }

  // Check supported version of PHP.
  define('E107CLI_MINIMUM_PHP', '5.3.0');
  if (version_compare(phpversion(), E107CLI_MINIMUM_PHP) < 0 && !getenv('E107CLI_NO_MIN_PHP')) {
    $msg = 'Your command line PHP installation is too old. E107cli requires at least PHP %s. To suppress this check, set the environment variable E107CLI_NO_MIN_PHP=1';
    return e107cli_error($msg, array(E107CLI_MINIMUM_PHP));
  }

  $version = e107cli_read_version();
  define('E107CLI_VERSION', $version['version']);
  $version_parts = explode('.', E107CLI_VERSION);
  define('E107CLI_MAJOR_VERSION', $version_parts[0]);
  define('E107CLI_MINOR_VERSION', $version_parts[1]);

  define('E107CLI_REQUEST_TIME', microtime(TRUE));

  e107cli_set_context('argc', $GLOBALS['argc']);
  e107cli_set_context('argv', $GLOBALS['argv']);

  // Set an error handler and a shutdown function
  // set_error_handler('e107cli_error_handler');
  register_shutdown_function('e107cli_shutdown');

  // We need some global options/arguments processed at this early stage.
  e107cli_parse_args();
}

/**
 * Cleanup our bootstrap.
 *
 * This is ran after we have bootstrapped and dispatched properly in
 * e107cli_main().
 *
 * @see e107cli_main()
 */
function e107cli_bootstrap_finish() {
  // We set this context to let the shutdown function know we reached the end
  // of e107cli_main();
  e107cli_set_context("E107CLI_EXECUTION_COMPLETED", TRUE);
}

/**
 * Bootstrap e107cli to the desired phase.
 *
 * This function will sequentially bootstrap each lower phase up to the phase
 * that has been requested.
 *
 * @param phase
 *   The bootstrap phase to bootstrap to. Any of the following constants :
 *     E107CLI_BOOTSTRAP_BASE = Only e107cli.
 *     E107CLI_BOOTSTRAP_E107 = Find a valid e107 root.
 *
 * @return mixed
 */
function e107cli_bootstrap($phase, $phase_max = FALSE) {
  static $phases;

  if (!$phases) {
    $phases = _e107cli_bootstrap_phases(TRUE);
  }

  static $phase_index = 0;

  e107cli_set_context('E107CLI_BOOTSTRAPPING', TRUE);

  while ($phase >= $phase_index && isset($phases[$phase_index])) {
    if (e107cli_bootstrap_validate($phase_index)) {
      $current_phase = $phases[$phase_index];

      if (function_exists($current_phase) && !e107cli_get_error()) {
        $current_phase();

        // Find any command files that are available during this bootstrap
        // phase.
        _e107cli_find_commandfiles($phase_index);
      }

      e107cli_set_context('E107CLI_BOOTSTRAP_PHASE', $phase_index);
    }
    else {
      $errors = e107cli_get_context('E107CLI_BOOTSTRAP_ERRORS', array());
      foreach ($errors as $code => $message) {
        e107cli_set_error($code, $message);
      }
    }

    unset($phases[$phase_index++]);
  }

  e107cli_set_context('E107CLI_BOOTSTRAPPING', FALSE);

  return !e107cli_get_error();
}

/**
 * Validate whether a bootstrap phases can be reached.
 *
 * This function will validate the settings that will be used during the actual
 * bootstrap process, and allow commands to progressively bootstrap to the
 * highest level that can be reached.
 *
 * This function will only run the validation function once, and store the
 * result from that execution in a local static. This avoids validating phases
 * multiple times.
 *
 * @param phase
 *   The bootstrap phase to bootstrap to. Any of the following constants :
 *     E107CLI_BOOTSTRAP_BASE = Only e107cli.
 *     E107CLI_BOOTSTRAP_E107 = Find a valid e107 root.
 *
 * @return
 *   True if bootstrap is possible, False if the validation failed.
 *
 */
function e107cli_bootstrap_validate($phase) {
  static $phases;
  static $result_cache = array();

  if (!$phases) {
    $phases = _e107cli_bootstrap_phases(TRUE);
  }

  static $phase_index = 0;

  if (!array_key_exists($phase, $result_cache)) {
    e107cli_set_context('E107CLI_BOOTSTRAP_ERRORS', array());
    e107cli_set_context('E107CLI_BOOTSTRAP_VALUES', array());

    while ($phase >= $phase_index && isset($phases[$phase_index])) {
      $current_phase = $phases[$phase_index] . '_validate';

      if (function_exists($current_phase)) {
        $result_cache[$phase_index] = $current_phase();
      }
      else {
        $result_cache[$phase_index] = TRUE;
      }

      e107cli_set_context('E107CLI_BOOTSTRAP_VALIDATION_PHASE', $phase_index);

      unset($phases[$phase_index++]);
    }
  }

  return $result_cache[$phase];
}

/**
 * Validate initial e107cli bootstrap phase.
 */
function _e107cli_bootstrap_base_validate() {
  // Test safe mode is off.
  if (ini_get('safe_mode')) {
    return e107cli_bootstrap_error('E107CLI_SAFE_MODE', 'PHP safe mode is activated. E107cli requires that safe mode is disabled.');
  }

  return TRUE;
}

function _e107cli_bootstrap_base() {

}

/**
 * Validate initial e107cli bootstrap phase.
 */
function _e107cli_bootstrap_e107_validate() {
  $cwd = e107cli_cwd(); // Get working directory.

  $e107_config = $cwd . '/e107_config.php';

  if (!file_exists($e107_config)) {
    return e107cli_bootstrap_error('E107CLI_E107_INIT', 'Missing e107_config.php file.');
  }

  $class2 = $cwd . '/class2.php';

  if (!file_exists($class2)) {
    return e107cli_bootstrap_error('E107CLI_E107_INIT', 'Missing class2.php file.');
  }

  return TRUE;
}

function _e107cli_bootstrap_e107() {
  $cwd = e107cli_cwd(); // Get working directory.
  $class2 = $cwd . '/class2.php';

  require_once $class2;
}

/**
 * Bootstrap to the specified phase.
 *
 * @param $max_phase_index
 *   Only attempt bootstrap to the specified level.
 *
 * @return boolean
 */
function e107cli_bootstrap_to_phase($max_phase_index) {
  $phases = _e107cli_bootstrap_phases();
  $result = TRUE;

  // Try to bootstrap to the maximum possible level, without generating errors.
  foreach ($phases as $phase_index) {
    if ($phase_index > $max_phase_index) {
      // Stop trying, since we achieved what was specified.
      break;
    }

    if (e107cli_bootstrap_validate($phase_index)) {
      if ($phase_index > e107cli_get_context('E107CLI_BOOTSTRAP_PHASE', -1)) {
        $result = e107cli_bootstrap($phase_index, $max_phase_index);
      }
    }
    else {
      $result = FALSE;
      break;
    }
  }

  return $result;
}

/**
 * Helper function to collect any errors that occur during the bootstrap process.
 * Always returns FALSE, for convenience.
 */
function e107cli_bootstrap_error($code, $message = NULL) {
  $errors = e107cli_get_context('E107CLI_BOOTSTRAP_ERRORS');
  $errors[$code] = $message;
  e107cli_set_context('E107CLI_BOOTSTRAP_ERRORS', $errors);
  return FALSE;
}

function _e107cli_bootstrap_global_options() {
  e107cli_set_context('E107CLI_DEBUG', e107cli_get_option('debug'));

  // Suppress colored logging if --nocolor option is explicitly given or if
  // terminal does not support it.
  $nocolor = (e107cli_get_option('nocolor', FALSE));
  if (!$nocolor) {
    // Check for colorless terminal.  If there is no terminal, then
    // 'tput colors 2>&1' will return "tput: No value for $TERM and no -T specified",
    // which is not numeric and therefore will put us in no-color mode.
    $colors = exec('tput colors 2>&1');
    $nocolor = !($colors === FALSE || (is_numeric($colors) && $colors >= 3));
  }

  e107cli_set_context('E107CLI_NOCOLOR', $nocolor);
}

function e107cli_shutdown() {
  if (!e107cli_get_context('E107CLI_EXECUTION_COMPLETED', FALSE)) {
    // We did not reach the end of the drush_main function,
    // this generally means somewhere in the code a call to exit(),
    // was made. We catch this, so that we can trigger an error in
    // those cases.
    $error_msg = "E107cli command terminated abnormally due to an unrecoverable error.";
    e107cli_set_error("E107CLI_NOT_COMPLETED", $error_msg);
  }

  $phase = e107cli_get_context('E107CLI_BOOTSTRAP_PHASE', E107CLI_BOOTSTRAP_BASE);
  if (e107cli_get_context('E107CLI_BOOTSTRAPPING', FALSE)) {
    switch ($phase) {
      case E107CLI_BOOTSTRAP_E107:
        ob_end_clean();
        e107cli_set_error('DRUSH_E107_BOOTSTRAP_ERROR');
        break;
    }
  }

  // This way e107cli_return_status will always be the last shutdown function
  // (unless other shutdown functions register shutdown functions...) and won't
  // prevent other registered shutdown functions from running by calling exit()
  // before they get a chance.
  register_shutdown_function('e107cli_return_status');
}

function e107cli_return_status() {
  exit((e107cli_get_error()) ? E107CLI_FRAMEWORK_ERROR : E107CLI_SUCCESS);
}
