<?php

/**
 * @file
 * Functions used by e107cli to query the environment and setting the current
 * configuration.
 *
 * Bootstrapping now occurs in bootstrap.inc.
 *
 * @see includes/bootstrap.inc
 */


/**
 * Verify that we are running PHP through the command line interface.
 *
 * This function is useful for making sure that code cannot be run via the web
 * server, such as a function that needs to write files to which the web server
 * should not have access to.
 *
 * @return boolean
 *   A boolean value that is true when PHP is being run through the command line,
 *   and false if being run through cgi or mod_php.
 */
function e107cli_verify_cli() {
  return (php_sapi_name() == 'cli' || (is_numeric($_SERVER['argc']) && $_SERVER['argc'] > 0));
}

/**
 * Read the e107cli VERSION file.
 */
function e107cli_read_version() {
  $version_file = dirname(__FILE__) . '/../..' . '/VERSION';
  return parse_ini_file($version_file);
}

/**
 * Checks if the operating system has bash.
 *
 * MinGW has bash, but PHP isn't part of MinGW and hence doesn't run in bash.
 */
function e107cli_has_bash($os = NULL) {
  return (e107cli_is_cygwin($os) && !e107cli_is_mingw($os)) || !e107cli_is_windows($os);
}

/**
 * Check if the operating system is Windows. This will return TRUE under DOS,
 * Powershell Cygwin and MSYSGIT shells, so test for the Windows variant FIRST
 * if you care.
 */
function e107cli_is_windows($os = NULL) {
  return _e107cli_test_os($os, array("WIN", "CYGWIN", "CWRSYNC", "MINGW"));
}

/**
 * Check if the operating system is Winodws running some variant of cygwin --
 * either Cygwin or the MSYSGIT shell. If you care which is which, test mingw
 * first.
 */
function e107cli_is_cygwin($os = NULL) {
  return _e107cli_test_os($os, array("CYGWIN", "CWRSYNC", "MINGW"));
}

/**
 * Check if the operating system is MinGW.
 */
function e107cli_is_mingw($os = NULL) {
  return _e107cli_test_os($os, array("MINGW"));
}

/**
 * Check if the operating system is OS X. This will return TRUE for Mac OS X
 * (Darwin).
 */
function e107cli_is_osx($os = NULL) {
  return _e107cli_test_os($os, array("DARWIN"));
}

function _e107cli_test_os($os, $os_list_to_check) {
  $os = _e107cli_get_os($os);

  foreach ($os_list_to_check as $test) {
    if (strtoupper(substr($os, 0, strlen($test))) == strtoupper($test)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Return the OS we are running under.
 *
 * @return string
 *   Linux
 *   WIN* (e.g. WINNT)
 *   CYGWIN
 *   MINGW* (e.g. MINGW32)
 */
function _e107cli_get_os($os = NULL) {
  // The special os "CWRSYNC" can be used to indicate that we are testing a path
  // that will be passed as an argument to cwRsync, which requires that the path
  // be converted to /cygdrive/c/path, even on DOS or Powershell.
  // The special os "RSYNC" can be used to indicate that we want to assume
  // "CWRSYNC" when cwrsync is installed, or default to the local OS otherwise.
  if (strtoupper($os) == "RSYNC") {
    $os = _e107cli_get_os("LOCAL");

    // For now we assume that cwrsync is always installed on Windows, and never
    // installed son any other platform.
    return e107cli_is_windows($os) ? "CWRSYNC" : $os;
  }

  // We allow "LOCAL" to document, in instances where some parameters are being
  // escaped for use on a remote machine, that one particular parameter will
  // always be used on the local machine.
  if (isset($os) && ($os != "LOCAL")) {
    return $os;
  }

  if (_e107cli_test_os(getenv("MSYSTEM"), array("MINGW"))) {
    return getenv("MSYSTEM");
  }

  // QUESTION: Can we differentiate between DOS and POWERSHELL? They appear to
  // have the same environment.
  // At the moment, it does not seem to matter; they behave the same from PHP.
  // At this point we will just return PHP_OS.
  return PHP_OS;
}

/**
 * Returns the current working directory.
 *
 * This is the directory as it was when e107cli was started, not the directory
 * we are currently in. For that, use getcwd() directly.
 */
function e107cli_cwd() {
  if ($path = e107cli_get_context('E107CLI_OLDCWD')) {
    return $path;
  }

  // We use PWD if available because getcwd() resolves symlinks, which could
  // take us outside of the Drupal root, making it impossible to find.
  // $_SERVER['PWD'] isn't set on windows and generates a Notice.
  $path = isset($_SERVER['PWD']) ? $_SERVER['PWD'] : '';
  if (empty($path)) {
    $path = getcwd();
  }

  // Convert windows paths.
  $path = _e107cli_convert_path($path);

  // Save original working dir case some command wants it.
  e107cli_set_context('E107CLI_OLDCWD', $path);

  return $path;
}

/**
 * Converts a Windows path (dir1\dir2\dir3) into a Unix path (dir1/dir2/dir3).
 * Also converts a cygwin "drive emulation" path (/cygdrive/c/dir1) into a
 * proper drive path, still with Unix slashes (c:/dir1).
 */
function _e107cli_convert_path($path) {
  $path = str_replace('\\', '/', $path);
  $path = preg_replace('/^\/cygdrive\/([A-Za-z])(.*)$/', '\1:\2', $path);
  return $path;
}
