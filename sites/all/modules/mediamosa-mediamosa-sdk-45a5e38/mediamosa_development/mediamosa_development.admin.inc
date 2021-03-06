<?php
// $Id$

/**
 * MediaMosa is Open Source Software to build a Full Featured, Webservice
 * Oriented Media Management and Distribution platform (http://mediamosa.org)
 *
 * Copyright (C) 2010 SURFnet BV (http://www.surfnet.nl) and Kennisnet
 * (http://www.kennisnet.nl)
 *
 * MediaMosa is based on the open source Drupal platform and
 * was originally developed by Madcap BV (http://www.madcap.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, you can find it at:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

/**
 * @file
 * Admin related forms and functions.
 */

// Create dummy class.
class mediamosa_rest_call_dummy {
  const TITLE = 'title';             // Title of the REST call (In English, required).
  const DESCRIPTION = 'description'; // Description of the REST call (In English, required).

  const VAR_IS_REQUIRED = 'is_required';   // default no.

  // Values VAR_IS_REQUIRED
  const VAR_IS_REQUIRED_YES = 'YES';
  const VAR_IS_REQUIRED_NO = 'NO';

  const VAR_DESCRIPTION = 'description';   // Description of the var (In English).

  const IS_APP_ADMIN = 'is_app_admin';

  const VAR_IS_INTERNAL_ONLY = 'internal_only'; // If true, then this variable is only accepted by internals.

  // Values VAR_IS_INTERNAL_ONLY
  const VAR_IS_INTERNAL_ONLY_YES = 'YES';
  const VAR_IS_INTERNAL_ONLY_NO = 'NO';

  const VAR_TYPE = 'type';

  const VAR_IS_ARRAY = 'is_array';         // default no.

  // Values VAR_IS_ARRAY
  const VAR_IS_ARRAY_YES = 'YES';        // Make it an array, if not.
  const VAR_IS_ARRAY_NO = 'NO';          // May not be array, if not, take 1st element.

  const VAR_ALLOWED_VALUES = 'a_allowed_values'; // Optional array for checking against allowed values.

  const VAR_DEFAULT_VALUE = 'default_value';
}

function _mediamosa_development_execute_rest_call($form_state) {

  $uri_method = isset($_SESSION['mediamosa_development_params']['uri']) ? $_SESSION['mediamosa_development_params']['uri'] : '';
  if (empty($uri_method)) {
    return '';
  }
  list($uri, $method) = explode('|', $uri_method);

  $var_setup = _mediamosa_development_get_var_setup($uri, $method);

  $data = $uri_rebuild = $uri_vars = $errors = array();

  // fill in the uri with the values (when needed).
  foreach (explode('/', $uri) as $uri_var) {
    $uri_var = trim($uri_var, '# ');
    if ($uri_var{0} == '$') {
      if (!isset($form_state['values']['_' . drupal_substr($uri_var, 1)]) || $form_state['values']['_' . drupal_substr($uri_var, 1)] == '') {
        $errors[] = t('Missing value for @name', array('@name' => drupal_substr($uri_var, 1)));
        continue;
      }

      // So we know the urivars.
      $uri_vars[] = drupal_substr($uri_var, 1);

      // To rebuild.
      $uri_rebuild[] = $form_state['values']['_' . drupal_substr($uri_var, 1)];
    }
    else {
      $uri_rebuild[] = $uri_var;
    }
  }

  // Rebuild uri.
  $uri = implode('/', $uri_rebuild);

  // Get the vars.
  foreach ($var_setup['vars'] as $name => $var) {
    if (!isset($form_state['values']['_' . $name]) || $form_state['values']['_' . $name] == '') {
      continue;
    }

    if (in_array($name, $uri_vars)) {
      continue;
    }

    // Store the value.
    $data[$name] = $form_state['values']['_' . $name];
  }

  $commands = array();

  if (!empty($errors)) {
    return implode(', ', $errors);
  }

  // Create mediamosa_connector.
  $mediamosa_connector = new mediamosa_connector();

  // Get response.
  $response = $mediamosa_connector->request($uri, array('method' => $method, 'data' => $data));

  if (empty($response->data)) {
    return t('Check mediamosa connector settings, no output received.');
  }

  return $response->data;
}

/**
 * Get the REST calls.
 */
function _mediamosa_development_get_rest_calls() {
  // When we are part of the MediaMosa installation, then use the REST calls
  // here.
  if (module_exists('mediamosa')) {
    $rest_calls = module_invoke_all('mediamosa_register_rest_call');

    $rest_call_options = array('' => 'Select Rest call');
    foreach ($rest_calls as $uri => $rest_call) {
      foreach ($rest_call as $method => $rest_call_data) {
        $rest_call_options[$uri . '|' . $method] = $uri . ' [' . $method . ']';
      }
    }

    return $rest_call_options;
  }

  // Ok, try to use connector to get the REST calls.
  // Create mediamosa_connector.
  $mediamosa_connector = new mediamosa_connector();

  // Get response.
  $response = mediamosa_connector::response2array($mediamosa_connector->request_get('rest', array('data' => array('limit' => 0))));

  $rest_call_options = array();

  if ($response && !empty($response['items']['item'])) {
    foreach ($response['items']['item'] as $item) {
      $rest_call_options[$item['uri'] . '|' . $item['method']] = $item['uri'] . ' [' . $item['method'] . ']';
    }
  }

  if (empty($rest_call_options)) {
    drupal_set_message(t('No REST calls found, check connector.'), 'error');
  }

  return $rest_call_options;
}

/**
 * Get the URI information.
 */
function _mediamosa_development_get_var_setup($uri, $method) {

  // Need to rebuild the var_setup from the REST/uri_id call.
  // Ok, try to use connector to get the REST calls.
  // Create mediamosa_connector.
  $mediamosa_connector = new mediamosa_connector();

  // Get response.
  $response = mediamosa_connector::response2array($mediamosa_connector->request_get('rest', array('data' => array('limit' => 0))));

  // Build the var_setup.
  $var_setup = array();

  if ($response && !empty($response['items']['item'])) {

    $response_item = FALSE;

    foreach ($response['items']['item'] as $item) {

      if ($item['uri'] == $uri && $item['method'] == $method) {
        // Find the item we need and get the REST call info.
        $response_item = mediamosa_connector::response2array($mediamosa_connector->request_get('rest/' . rawurlencode($item['uri_id'])));
        break;
      }
    }
  }

  // Not found?
  if (empty($response_item['items']['item'])) {
    drupal_set_message(t('Was not able to find REST call, check MediaMosa Connector settings.'), 'error');
    return array();
  }

  // Get the item.
  return reset($response_item['items']['item']);
}


/**
 * The form to execute a REST call.
 */
function mediamosa_development_rest_form(&$form_state) {

  // Get session.
  $session = isset($_SESSION['mediamosa_development_params']) ? $_SESSION['mediamosa_development_params'] : array();

  drupal_add_css(drupal_get_path('module', 'mediamosa_development') . '/mediamosa_development.css');

  // Get all the REST calls.
  $rest_call_options = _mediamosa_development_get_rest_calls();

  ksort($rest_call_options);

  $form = array();

  $form['rest'] = array(
    '#type' => 'fieldset',
    '#title' => t('REST call'),
    '#weight' => -10,
  );

  if (empty($session['uri'])) {
    $form['rest']['uri'] = array(
      '#type' => 'select',
      '#title' => 'REST call URI',
      '#options' => $rest_call_options,
      '#default_value' => '',
    );

    $form['rest']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Select'),
    );
  }
  else {
    list($uri, $method) = explode('|', $session['uri']);

    $var_setup = _mediamosa_development_get_var_setup($uri, $method);

    $link = t('@uri [@method]', array('@uri' => $uri, '@method' => $method));

    $descriptions = array(
      !empty($var_setup[mediamosa_rest_call_dummy::TITLE]) ? '<b>' . $var_setup[mediamosa_rest_call_dummy::TITLE] . '</b>' : t('No title found'),
      !empty($var_setup[mediamosa_rest_call_dummy::DESCRIPTION]) ? $var_setup[mediamosa_rest_call_dummy::DESCRIPTION] : t('No description found'),
      '',
      $link,
    );

    $form['rest']['description'] = array(
      '#value' => '<p>' . implode('<br />', $descriptions) . '</p>',
    );

    $form['rest']['clear'] = array(
      '#type' => 'submit',
      '#value' => t('Clear URI selection'),
    );

    $form['rest']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Execute @uri [@method]', array('@uri' => $uri, '@method' => $method)),
      '#weight' => 10,
    );

    $form['rest_result'] = array(
      '#type' => 'fieldset',
      '#title' => t('REST Call output'),
      '#weight' => 10,
      '#prefix' => '<div id="rest_call_output">',
      '#suffix' => '</div>',
    );

    $form['rest_result']['output'] = array(
      '#type' => 'textarea',
      '#title' => t('Output'),
      '#default_value' => empty($form_state['storage']['rest_call_execute']) ? '' : _mediamosa_development_execute_rest_call($form_state),
    );

    $form = _mediamosa_build_form($form, $form_state);
  }

  return $form;
}

/**
 * Submit of the rest form.
 */
function mediamosa_development_rest_form_submit($form, &$form_state) {

  list($uri, $method) = explode('|', empty($_SESSION['mediamosa_development_params']['uri']) ? 'foo|foo' : $_SESSION['mediamosa_development_params']['uri']);

  switch ($form_state['values']['op']) {
    case t('Clear URI selection'):
      $_SESSION['mediamosa_development_params']['uri'] = '';
      unset($form_state['storage']['rest_call_execute']);
      break;

    case t('Select'):
      // Set uri in session.
      $_SESSION['mediamosa_development_params']['uri'] = empty($form_state['values']['uri']) ? '' : $form_state['values']['uri'];
      break;

    case t('Execute @uri [@method]', array('@uri' => $uri, '@method' => $method)):
      $form_state['storage']['rest_call_execute'] = TRUE;
      break;
  }
}

function _mediamosa_build_form($form, $form_state) {

  if (empty($_SESSION['mediamosa_development_params']['uri'])) {
    return $form;
  }

  list($uri, $method) = explode('|', $_SESSION['mediamosa_development_params']['uri']);
  $var_setup = _mediamosa_development_get_var_setup($uri, $method);

  // Clear them.
  $required = $optional = array();

  $skip = array('app_id');

  // Sort vars on name.
  ksort($var_setup['vars']);

  // First the required.
  $weight = 0;
  foreach ($var_setup['vars'] as $name => $var) {
    if (!isset($var[mediamosa_rest_call_dummy::VAR_IS_REQUIRED]) || $var[mediamosa_rest_call_dummy::VAR_IS_REQUIRED] == mediamosa_rest_call_dummy::VAR_IS_REQUIRED_NO) {
      continue;
    }

    // Some need to be skipped.
    if (in_array($name, $skip)) {
      continue;
    }

    // Skip it when its not used. @todo remove
    if ($name == mediamosa_rest_call_dummy::IS_APP_ADMIN && $var[mediamosa_rest_call_dummy::VAR_DESCRIPTION] == 'NOT USED.') {
      continue;
    }

    $item = _build_param($name, $var, TRUE);
    if (isset($form_state['values']['_' . $name])) {
      $item['#default_value'] = $form_state['values']['_' . $name];
    }

    if (!empty($item)) {
      $item['#weight'] = $weight++;
      $required['_' . $name] = $item; // prefix with '_' so we dont have problems with double names.
    }
  }

  // Second the optional.
  $weight = 0;
  foreach ($var_setup['vars'] as $name => $var) {
    if (isset($var[mediamosa_rest_call_dummy::VAR_IS_REQUIRED]) && $var[mediamosa_rest_call_dummy::VAR_IS_REQUIRED] == mediamosa_rest_call_dummy::VAR_IS_REQUIRED_YES) {
      continue;
    }

    // Some need to be skipped.
    if (in_array($name, $skip)) {
      continue;
    }

    // Skip it when its not used. @todo remove
    if ($name == mediamosa_rest_call_dummy::IS_APP_ADMIN && $var[mediamosa_rest_call_dummy::VAR_DESCRIPTION] == 'NOT USED.') {
      continue;
    }

    $item = _build_param($name, $var, FALSE);
    if (isset($form_state['values']['_' . $name])) {
      $item['#default_value'] = $form_state['values']['_' . $name];
    }

    if (!empty($item)) {
      $item['#weight'] = $weight++;
      $optional['_' . $name] = $item;
    }
  }

  if (empty($required) && empty($optional)) {
    $form['rest']['required'] = array('#value' => '<p>' . t('No parameters for this REST call.') . '</p>');
  }
  else {
    if (!empty($required)) {
      $form['rest']['required'] = array(
        '#type' => 'fieldset',
        '#title' => t('Required parameters'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      );

      $form['rest']['required'] = isset($form['rest']['required']) ? array_merge($required, $form['rest']['required']) : $required;
    }
    else {
      $form['rest']['required'] = array('#value' => '<p>' . t('No required parameters.') . '</p>');
    }

    if (!empty($optional)) {
      $form['rest']['optional'] = array(
        '#type' => 'fieldset',
        '#title' => t('Optional parameters'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );

      $form['rest']['optional'] = array_merge($optional, $form['rest']['optional']);
    }
  }

  return $form;
}

/**
 * Helper for creating param form item.
 *
 * @param string $name
 * @param array $var
 */
function _build_param($name, $var, $required) {
  $item = array();

  // For now skip internal vars.
  if (isset($var[mediamosa_rest_call_dummy::VAR_IS_INTERNAL_ONLY]) && $var[mediamosa_rest_call_dummy::VAR_IS_INTERNAL_ONLY] == mediamosa_rest_call_dummy::VAR_IS_INTERNAL_ONLY_YES) {
    return '';
  }

  $type = mediamosa_sdk::typeAlias2Type($var[mediamosa_rest_call_dummy::VAR_TYPE]);
  $is_array = isset($var[mediamosa_rest_call_dummy::VAR_IS_ARRAY]) && $var[mediamosa_rest_call_dummy::VAR_IS_ARRAY] == mediamosa_rest_call_dummy::VAR_IS_ARRAY_YES;

  // Title.
  $title = $name . ($is_array ? '[]' : '');
  $options = array();

  if (!empty($var[mediamosa_rest_call_dummy::VAR_ALLOWED_VALUES])) {
    if (!$required) {
      $options[''] = t('Make selection');
      $var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE] = t('Make selection');
    }

    foreach ($var[mediamosa_rest_call_dummy::VAR_ALLOWED_VALUES] as $value) {
      $options[$value] = $value;
    }

    // Go pass the type.
    $type = 'hack_selection';
  }

  // Description.
  $description = $var[mediamosa_rest_call_dummy::VAR_DESCRIPTION];

  switch ($type) {
    case 'hack_selection':
      $item = array(
        '#weight' => 10,
        '#type' => 'select',
        '#title' => $title,
        '#description' => $description,
        '#options' => $options,
      );
      break;

    case mediamosa_sdk::TYPE_BOOL:
      $item = array(
        '#weight' => 10,
        '#type' => 'select',
        '#title' => $title,
        '#description' => $description,
        '#options' => array('' => '', 'false' => t('false'), 'true' => t('true')),
      );

      if (isset($var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE])) {
        $var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE] = drupal_strtolower($var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE]);
      }

      break;

    case mediamosa_sdk::TYPE_LANGUAGE_CODE:
      $options = explode(',', 'aa,ab,ae,af,ak,am,an,ar,as,av,ay,az,ba,be,bg,bh,bi,bm,bn,bo,br,bs,ca,ce,ch,co,cr,cs,cu,cv,cy,da,de,dv,dz,ee,el,en,eo,es,et,eu,fa,ff,fi,fj,fo,fr,fy,ga,gd,gl,gn,gu,gv,ha,he,hi,ho,hr,ht,hu,hy,hz,ia,id,ie,ig,ii,ik,io,is,it,iu,ja,jv,ka,kg,ki,kj,kk,kl,km,kn,ko,kr,ks,ku,kv,kw,ky,la,lb,lg,li,ln,lo,lt,lu,lv,mg,mh,mi,mk,ml,mn,mo,mr,ms,mt,my,na,nb,nd,ne,ng,nl,nn,no,nr,nv,ny,oc,oj,om,or,os,pa,pi,pl,ps,pt,qu,rm,rn,ro,ru,rw,ry,sa,sc,sd,se,sg,sh,si,sk,sl,sm,sn,so,sq,sr,ss,st,su,sv,sw,ta,te,tg,th,ti,tk,tl,tn,to,tr,ts,tt,tw,ty,ug,uk,ur,uz,ve,vi,vo,wa,wo,xh,yi,yo,za,zh,zu');
      $item = array(
        '#type' => 'select',
        '#title' => $title,
        '#description' => $description,
        '#options' => $options,
      );
      break;

    case mediamosa_sdk::TYPE_APP_ID:
    case mediamosa_sdk::TYPE_INT:
    case mediamosa_sdk::TYPE_UINT:
    case mediamosa_sdk::TYPE_AUT_GROUP_TYPE:
    case mediamosa_sdk::TYPE_TICKET_ID:

    case mediamosa_sdk::TYPE_URI:
    case mediamosa_sdk::TYPE_FILENAME:
    case mediamosa_sdk::TYPE_STRING:
    case mediamosa_sdk::TYPE_ALPHA:
    case mediamosa_sdk::TYPE_ALPHA_NUM:
    case mediamosa_sdk::TYPE_ALPHA_NUM_UNDERSCORE:
    case mediamosa_sdk::TYPE_ALPHA_NUM_UNDERSCORE_TAG:

    case mediamosa_sdk::TYPE_DATETIME:

    case mediamosa_sdk::TYPE_SERIAL:
    case mediamosa_sdk::TYPE_URL:
    case mediamosa_sdk::TYPE_JOB:
    case mediamosa_sdk::TYPE_COMMAND:
      $item = array(
        '#weight' => '10',
        '#type' => 'textfield',
        '#title' => $title,
        '#description' => $description,
      );

      break;
    default:
      drupal_set_message(t('No form creation found for @name with type @type.', array('@name' => $name, '@type' => $type)), 'error');
      break;
  }

  if (!empty($item)) {
    if (!empty($var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE])) {
      $item['#default_value'] = $var[mediamosa_rest_call_dummy::VAR_DEFAULT_VALUE];
    }
  }
  return $item;
}
