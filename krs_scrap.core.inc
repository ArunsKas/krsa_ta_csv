<?php

global $url_base;

function _write_to_csv($data, $filename) {
  $path = 'public://csv/';
  // Ensure the directory
  if (!file_prepare_directory($path, FILE_CREATE_DIRECTORY)) {
    drupal_set_message(t('Unable to create directory in file system. Check permissions and try again.'), 'error');
    return;
  }
  // Ensure the file
  $file = file_save_data('', $path . $filename);
  if (!$file) {
    drupal_set_message(t('Unable to write to file system. Check permissions and try again.'), 'error');
    return;
  }
  // Stream data - This is a simplified example. Better to do it in a batch if you have a high volume of data
  $fh = fopen($file->uri, 'w');
  if (!$fh) {
    drupal_set_message(t('Unable open file for writing. Check permissions and try again.'), 'error');
    return;
  }

  fputcsv($fh, array(
    'Title',
    'Link',
  ));

  foreach ($data as $group_items) {
    foreach ($group_items as $item) {
      fputcsv($fh, array(
        $item['title'],
        $item['href'],
      ));
    }
  }
  fclose($fh);

  // Notify the filesystem of the size change
  $file->filesize = filesize($file->uri);
  file_save($file);
  // Tell the user where we stuck it
  drupal_set_message(t('New csv file: <a href="!url">!filename</a>.', array(
    '!url' => file_create_url($file->uri),
    '!filename' => $filename,
  )));
}

function _csv_files_table($register) {
  $public_path = variable_get('file_public_path', conf_path());
  $path = $public_path . '/csv';
  $mask = '/\.csv$/';
  //"/\\". $register ."-([0-9]*)/";
  $csv_files = file_scan_directory($path, $mask);

  $header = array(
    array('data' => t('Filename')),
    array('data' => t('Filesize')),
    array('data' => t('Last Update')),
    array('data' => t('Operations'))
  );

  $rows = array();

  $files_objects_arr = array();
  foreach ($csv_files as $file) {
    $uri = 'public://' . ltrim($file->uri, $public_path);
    if ($file_obj = _file_load_by_uri($uri)) {
      $files_objects_arr[] = $file_obj;
    }
  }

  // sort by last update time
  uasort($files_objects_arr, '__sort_callback');

  foreach ($files_objects_arr as $file_obj) {
    $update = format_date($file_obj->timestamp, $type = 'medium');
    $filename = drupal_basename($file_obj->uri);
    $file_link = l($filename, file_create_url($file_obj->uri));
    $filesize = filesize($file_obj->uri);

    preg_match('/-scrap.*.csv$/', $filename, $matches);
    if ($matches && $matches[0]) {
      $operations = l(t('delete'), 'to_csv/delete/' . $file_obj->fid);
    }
    else {
      preg_match('/-parsed.*.csv$/', $filename, $matches);
      if ($matches && $matches[0]) {
        $operations = l(t('delete'), 'to_csv/delete/' . $file_obj->fid);
      }
      else {
        $operations = '';
      }
    }

    $rows[] = array(
      array('data' => $file_link),
      array('data' => $filesize),
      array('data' => $update),
      array('data' => $operations)
    );
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

function _clear_text_string($text) {
  // remove spaces from start and end
  $text = trim($text);
  // remove dot from start and end '.'
  $text = trim($text, '.');
  // decode special chars like '&quot;'
  $text = htmlspecialchars_decode($text);
  $text = decode_entities($text);
  // remove double and more spaces 
  $text = preg_replace("/ {2,}/", " ", $text);
  // remove string '(neskelbiamas)' from end
  $text = str_replace('(neskelbiamas)', '', $text);
  // remove spaces from start and end
  $text = trim($text);
  // remove start german quote
  $text = trim($text, '„');
  // remove end german quote
  $text = trim($text, '“');
  // remove double quote '"' from start and end
  $text = trim($text, '"');
  // replace ' " ' (space double quote space) to '" '
  $text = str_replace(' " ', '" ', $text);
  // fix number of open close quotes  
  $text = _fix_quotes($text);
  return $text;
}

/**
 * fix number of open close quotes  
 * @param type $text
 * @return string
 */
function _fix_quotes($text) {
  $q1 = '"';
  $q2 = '„';
  $q3 = '“';
  $pos_q1 = strpos($text, $q1);
  if ($pos_q1) {
    // convert all quotes to q1 type if quote of q1 type found in string
    $text = str_replace($q2, $q1, $text);
    $text = str_replace($q3, $q1, $text);
    $count_q1 = substr_count($text, $q1);
    if ($count_q1 % 2 <> 0) {
      // append q1 to end of string
      $text = $text . $q1;
    }
    else {
      // check requirement and add missing quote at end if needed
      $open_q1_count = substr_count($text, ' "');
      $total_q1_count = substr_count($text, '"');
      if ($total_q1_count - $open_q1_count < $open_q1_count) {
        $text = $text . $q1;
      }
    }

    // replace all ' "' to opening quote
    $text = str_replace(' "', ' ' . $q2, $text);
    // replace all '" ' to closing quote
    $text = str_replace('" ', $q3 . ' ', $text);

    // replace all '"' at string end to closing quote
    $last_char = substr($text, -1);
    if ($last_char == '"') {
      $text = substr($text, 0, -1) . $q3;
    }
  }
  else {
    $count_q2 = substr_count($text, $q2);
    $count_q3 = substr_count($text, $q3);
    if ($count_q2 - $count_q3 == 1) {
      // append missing q3
      $text = $text . $q3;
    }
  }

//  $space_q1_pos = strpos($text, ' "');
//  $space_q1_count = substr_count($text, ' "');
//  $quote_count = substr_count($text, '"');
//  if (($space_q1_count >= 2) ) {
//    dd($space_q1_count. ' ' . $text);
//    dd($text);
//  }
//  $pattern = '/([\s\\"]{1,})/';
//  preg_match_all($pattern, $text, $matches);
//  if (!empty($matches) && isset($matches[0])) {
//    kpr($matches);
//  }

  return $text;
}

function _similar_to_main_doc($title, $needle) {
  $title_low = strtolower($title);
  $pos = strpos($title_low, $needle);
  if (($pos > 10) && ($pos < 30)) {
    return true;
  }
  return false;
}

/**
 * Function parse raw array data from scraper to strict structure:
 * - new row per month / scrapped page
 * - each sub row can be main document or extra document (attachment to main doc)
 * - each document can have link to file or link can be empty
 * - each document must have title
 * @param array $all_docs_info - multidimensional array of scrapped tables rows
 * @return array - multidimensional parsed structured array
 */
function _parse_all_doc_info($all_docs_info) {

  $parsed_data = array();
  foreach ($all_docs_info as $key => $docs_per_month) {
    $count_a = count($docs_per_month['a']);
    $count_p = count($docs_per_month['p']);

    $docs_per_month_result = $docs_per_month['a'];
    if ($count_p > $count_a) {
      // if some records don't have links to file

      $tmp_a = array();
      foreach ($docs_per_month['a'] as $item_a) {
        $tmp_a[] = $item_a['title'];
      }

      // create array with records without links
      $arr1 = $docs_per_month['p'];
      $arr2 = $tmp_a;
      $without_links_arr = _array_diff_by_str_val($arr1, $arr2);

      // insert records without links in array whose records are only with links
      foreach ($without_links_arr as $index => $value) {
        array_splice($docs_per_month_result, $index, 0, array($value));
      }
    }
    else {
      // all records have links - nothing to do.      
    }
    $parsed_data[$key] = $docs_per_month_result;
  }
  return $parsed_data;
}

// create array with records without links
// function is similar to array_diff()
function _array_diff_by_str_val($arr1, $arr2) {
  $pos_shift = 0;
  $array_diff = array();
  foreach ($arr1 as $pos => $item) {
    if (isset($arr2[$pos + $pos_shift])) {
      if (!($item == $arr2[$pos + $pos_shift])) {
        $array_diff[$pos] = array('title' => $item, 'href' => '');
        $pos_shift--;
      }
    }
    else {
      $array_diff[$pos] = array('title' => $item, 'href' => '');
    }
  }
  return $array_diff;
}

/**
 * Get HTTP_REFERER as Drupal normal path without language prefix.
 */
function referer_path() {
  if (!empty($_SERVER['HTTP_REFERER'])) {
    $url = parse_url($_SERVER['HTTP_REFERER']);
    if ($url['path'][0] == '/') {
      $url['path'] = substr($url['path'], 1);
    }
    //list(, $path) = language_url_split_prefix($url['path'], language_list());
    return $url['path'];
  }
}

/**
 * function to return first match of dddd (four digit year)
 */
function _get_doc_year($txt) {
  $pattern = '/([0-9]{4})/';
  preg_match_all($pattern, $txt, $matches);
  return $matches[0][0];
}

/**
 * Function to extract month in two digit format from full date string 
 * @param type $txt - string of style '2017 m. sausio 30 d.'
 * @param type $y - string '2017'
 * @param type $d - string '30'
 * @return string - for example string return '01'
 */
function _get_doc_month($txt, $y, $d) {
  // cut first year four digits from text 
  $txt_without_y = substr($txt, strpos($txt, $y) + 4, strlen($txt));
  // cut text after two day digits
  $day_pos = strpos($txt_without_y, ltrim($d, '0'));
  $txt1 = substr($txt_without_y, 0, $day_pos);
  $txt2 = trim(ltrim(trim(str_replace($y, '', $txt1)), 'm.'));

  switch ($txt2) {
    case 'sausio':
      $result = '01';
      break;
    case 'vasario':
      $result = '02';
      break;
    case 'kovo':
      $result = '03';
      break;
    case 'balandžio':
      $result = '04';
      break;
    case 'gegužės':
      $result = '05';
      break;
    case 'birželio':
      $result = '06';
      break;
    case 'liepos':
      $result = '07';
      break;
    case 'rugpjūčio':
      $result = '08';
      break;
    case 'rugsėjo':
      $result = '09';
      break;
    case 'spalio':
      $result = '10';
      break;
    case 'lapkričio':
      $result = '11';
      break;
    case 'gruodžio':
      $result = '12';
      break;
    default:
      $result = '';
      break;
  }
  return $result;
}

function _get_doc_day($txt) {
  $pattern = '/(\s|[a-z]*)\b([0-9]{1,2})\b/';
  preg_match_all($pattern, $txt, $matches);
  return str_pad(trim($matches[0][0]), 2, "0", STR_PAD_LEFT);
}

/**
 * function to return first match of {$register}-ddd
 */
function _get_doc_no($txt, $register) {
  $pattern = "/\\". $register ."-([0-9]*)/";
  preg_match_all($pattern, $txt, $matches);
  return $matches[0][0];
}

/**
 * Load file object by uri. This function is missing in drupal file api
 */
function _file_load_by_uri($uri) {
  $query = db_select('file_managed', 'fm');
  $query->condition('uri', $uri, '=');
  $query->fields('fm', array('fid'));
  $result = $query->execute()->fetchCol();
  if (!empty($result) && ($fid = $result[0])) {
    return file_load($fid);
  }
  else {
    return FALSE;
  }
}

/**
 * custom callback for php uasort function
 */
function __sort_callback($a, $b) {
  if ($a->fid == $b->fid) {
    return 0;
  }
  return ($a->fid > $b->fid) ? -1 : 1;
}
