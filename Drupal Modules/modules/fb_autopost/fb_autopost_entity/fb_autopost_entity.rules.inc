<?php

/**
 * @file
 * Rules integration
 */

/**
 * Implements hook_rules_action_info().
 */
function fb_autopost_entity_rules_action_info() {
  $actions['publish_to_facebook'] = array(
    'label' => t('Publish to Facebook page'),
    'group' => t('Facebook Autopost'),
    'parameter' => array(
      'publication' => array(
        'type' => 'facebook_publication',
        'label' => t('Facebook publication'),
      ),
      'pages' => array(
        'type' => 'list<text>',
        'label' => t('Facebook pages'),
        'options list' => 'available_facebook_pages',
        'access callback' => 'can_administer_facebook_publications',
      ),
    ),
    'base' => 'rules_action_publish_to_facebook',
    'provides' => array(
      'facebook_publication_id' => array(
        'type' => 'text',
        'label' => t('Facebook publication returned ID'),
      )
    ),
  );
  $actions['publish_to_facebook_timeline'] = $actions['publish_to_facebook'];
  $actions['publish_to_facebook_timeline']['label'] = t('Publish to Facebook user\'s timeline');
  $actions['publish_to_facebook_timeline']['base'] = 'rules_action_publish_to_facebook_timeline';
  unset($actions['publish_to_facebook_timeline']['parameter']['pages']);
  return $actions;
}

/**
 * Access callback for users with permission to 'administer facebook publications'
 */
function can_administer_facebook_publications() {
  return user_access('administer facebook publications');
}

/**
 * Returns the available options for the $page paramenter
 */
function available_facebook_pages() {
  module_load_include('inc', 'fb_autopost', 'fb_autopost.admin');
  $available = array_values(array_filter(variable_get('fb_autopost_page', array())));
  try {
    $fb = new FBAutopost();
    $pages_data = $fb->getPagesData(variable_get('fb_autopost_account_id', 'me'), variable_get('fb_autopost_token', ''));
    $options = _get_page_options($pages_data);
    foreach ($options as $option => $label) {
      if (in_array($option, $available)) {
        $output[$option] = filter_xss($label);
      }
    }
    return $output;
  } catch (Exception $e) {
    watchdog('fb_autopost', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Callback function called when executing the action.
 */
function rules_action_publish_to_facebook($publication, $pages = 'all') {
  try {
    $fb = new FBAutopost();
    $facebook_returned_id = $fb->entity_publish($publication, $pages);
    return array('facebook_publication_id' => $facebook_returned_id['id']);
  } catch (ErrorException $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Callback function called when executing the action.
 */
function rules_action_publish_to_facebook_timeline($publication) {
  rules_action_publish_to_facebook($publication, 'me');
}
