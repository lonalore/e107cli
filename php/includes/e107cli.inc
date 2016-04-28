<?php

/**
 * @file
 * The e107cli API implementation and helpers.
 */


/**
 * @name Error status definitions
 * @{
 * Error code definitions for interpreting the current error status.
 * @see e107cli_set_error(), e107cli_get_error()
 */

/**
 * The command completed successfully.
 */
define('E107CLI_SUCCESS', 0);

/**
 * The command could not be completed because the framework has specified errors
 * that have occurred.
 */
define('E107CLI_FRAMEWORK_ERROR', 1);

/**
 * @} End of "name Error status defintions".
 */


/**
 * E107cli error handler.
 *
 * @param string $message
 *   Error message.
 * @param array $args
 *   Arguments.
 *
 * @return mixed
 */
function e107cli_error($message = '', $args = array()) {
  call_user_func_array('printf', array_merge((array) $message, $args));
  return FALSE;
}

/**
 * Set an error code for the error handling system.
 *
 * @param error
 *   A text string identifying the type of error.
 *
 * @param message
 *   Optional. Error message to be logged. If no message is specified, hook_drush_help will be consulted,
 *   using a key of 'error:MY_ERROR_STRING'.
 *
 * @return boolean
 *   Always returns FALSE, to allow you to return with false in the calling functions,
 *   such as <code>return drush_set_error('DRUSH_FRAMEWORK_ERROR')</code>
 */
function e107cli_set_error($error, $message = NULL) {
  $error_code = &e107cli_get_context('E107CLI_ERROR_CODE', E107CLI_SUCCESS);
  $error_code = E107CLI_FRAMEWORK_ERROR;

  // TODO: log

  return FALSE;
}

/**
 * Return the current error handling status.
 *
 * @return
 *   The current aggregate error status.
 */
function e107cli_get_error() {
  return e107cli_get_context('E107CLI_ERROR_CODE', E107CLI_SUCCESS);
}

/**
 * Prints a message with optional indentation. In general,
 * e107cli_log($message, 'ok') is often a better choice than this function.
 * That gets your confirmation message (for example) into the logs for this
 * e107cli request.
 *
 * @param $message
 *   The message to print.
 * @param $indent
 *    The indentation (space chars)
 */
function e107cli_print($message = '', $indent = 0) {
  $msg = str_repeat(' ', $indent) . (string) $message . "\n";
  if ($charset = e107cli_get_option('output_charset') && function_exists('iconv')) {
    $msg = iconv('UTF-8', $charset, $msg);
  }
  print $msg;
}

/**
 * Stores a message which is printed during e107cli_shutdown() if in compact
 * mode.
 *
 * @param $message
 *   The message to print. If $message is an array, then each element of the
 *   array is printed on a separate line.
 */
function e107cli_print_pipe($message = '') {
  $buffer = &e107cli_get_context('E107CLI_PIPE_BUFFER', '');
  if (is_array($message)) {
    $message = implode("\n", $message) . "\n";
  }
  $buffer .= $message;
}

/**
 * Prints an array or string.
 *
 * @param $array
 *   The array to print.
 */
function e107cli_print_r($array) {
  print_r($array);
}

/**
 * Print a formatted table.
 *
 * @param $rows
 *   The rows to print.
 * @param $header
 *   If TRUE, the first line will be treated as table header and therefore be
 *   underlined.
 * @param $widths
 *   The widths of each column (in characters) to use - if not specified this
 *   will be determined automatically, based on a "best fit" algorithm.
 *
 * @return string
 */
function e107cli_print_table($rows, $header = FALSE, $widths = array()) {
  $tbl = new Console_Table(CONSOLE_TABLE_ALIGN_LEFT, '');

  $auto_widths = e107cli_table_column_autowidth($rows, $widths);

  // Do wordwrap on all cells.
  $newrows = array();

  foreach ($rows as $rowkey => $row) {
    foreach ($row as $col_num => $cell) {
      $newrows[$rowkey][$col_num] = wordwrap($cell, $auto_widths[$col_num], "\n", TRUE);
      if (isset($widths[$col_num])) {
        $newrows[$rowkey][$col_num] = str_pad($newrows[$rowkey][$col_num], $widths[$col_num]);
      }
    }
  }

  if ($header) {
    $headers = array_shift($newrows);
    $tbl->setHeaders($headers);
  }

  $tbl->addData($newrows);

  e107cli_print($tbl->getTable());

  return $tbl;
}


/**
 * Determine the best fit for column widths.
 *
 * @param $rows
 *   The rows to use for calculations.
 * @param $widths
 *   Manually specified widths of each column (in characters) - these will be
 *   left as is.
 */
function e107cli_table_column_autowidth($rows, $widths) {
  $auto_widths = $widths;

  // First we determine the distribution of row lengths in each column.
  // This is an array of descending character length keys (i.e. starting at
  // the rightmost character column), with the value indicating the number
  // of rows where that character column is present.
  $col_dist = array();
  foreach ($rows as $rowkey => $row) {
    foreach ($row as $col_num => $cell) {
      if (empty($widths[$col_num])) {
        $length = strlen($cell);
        while ($length > 0) {
          if (!isset($col_dist[$col_num][$length])) {
            $col_dist[$col_num][$length] = 0;
          }
          $col_dist[$col_num][$length]++;
          $length--;
        }
      }
    }
  }
  foreach ($col_dist as $col_num => $count) {
    // Sort the distribution in decending key order.
    krsort($col_dist[$col_num]);
    // Initially we set all columns to their "ideal" longest width
    // - i.e. the width of their longest column.
    $auto_widths[$col_num] = max(array_keys($col_dist[$col_num]));
  }

  // We determine what width we have available to use, and what width the
  // above "ideal" columns take up.
  $available_width = e107cli_get_context('E107CLI_COLUMNS', 80) - (count($auto_widths) * 2);
  $auto_width_current = array_sum($auto_widths);

  // If we need to reduce a column so that we can fit the space we use this
  // loop to figure out which column will cause the "least wrapping",
  // (relative to the other columns) and reduce the width of that column.
  while ($auto_width_current > $available_width) {
    $count = 0;
    $width = 0;
    foreach ($col_dist as $col_num => $counts) {
      // If we are just starting out, select the first column.
      if ($count == 0 ||
        // OR: if this column would cause less wrapping than the currently
        // selected column, then select it.
        (current($counts) < $count) ||
        // OR: if this column would cause the same amount of wrapping, but is
        // longer, then we choose to wrap the longer column (proportionally
        // less wrapping, and helps avoid triple line wraps).
        (current($counts) == $count && key($counts) > $width)
      ) {
        // Select the column number, and record the count and current width
        // for later comparisons.
        $column = $col_num;
        $count = current($counts);
        $width = key($counts);
      }
    }
    if ($width <= 1) {
      // If we have reached a width of 1 then give up, so wordwrap can still progress.
      break;
    }

    // Reduce the width of the selected column.
    $auto_widths[$column]--;
    // Reduce our overall table width counter.
    $auto_width_current--;
    // Remove the corresponding data from the disctribution, so next time
    // around we use the data for the row to the left.
    unset($col_dist[$column][$width]);
  }

  return $auto_widths;
}

/**
 * Executes a shell command.
 * Output is only printed if in verbose mode.
 * Output is stored and can be retrieved using e107cli_shell_exec_output().
 * If in simulation mode, no action is taken.
 *
 * @param $cmd
 *   The command to execute. May include placeholders used for sprintf.
 * @param ...
 *   Values for the placeholders specified in $cmd. Each of these will be
 *   passed through escapeshellarg() to ensure they are safe to use on the
 *   command line.
 *
 * @return mixed
 *   0 if success.
 */
function e107cli_shell_exec($cmd) {
  $args = func_get_args();

  // Do not change the command itself, just the parameters.
  for ($x = 1; $x < sizeof($args); $x++) {
    $args[$x] = escapeshellarg($args[$x]);
  }

  $command = call_user_func_array('sprintf', $args);

  if (!e107cli_get_context('E107CLI_SIMULATE')) {
    exec($command . ' 2>&1', $output, $result);
    _e107cli_shell_exec_output_set($output);

    if (e107cli_get_context('E107CLI_DEBUG')) {
      foreach ($output as $line) {
        e107cli_print($line, 2);
      }
    }

    // Exit code 0 means success.
    return ($result == 0);
  }
  else {
    return 0;
  }
}

/**
 * Stores output for the most recent shell command.
 * This should only be run from e107cli_shell_exec().
 *
 * @param boolean $output
 *   The output of the most recent shell command.
 *   If this is not set the stored value will be returned.
 *
 * @return mixed
 */
function _e107cli_shell_exec_output_set($output = FALSE) {
  static $stored_output;
  if ($output === FALSE) {
    return $stored_output;
  }
  $stored_output = $output;
}
