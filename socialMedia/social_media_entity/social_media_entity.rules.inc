<?php

/**
 * @file
 * Rules integration.
 */

/**
 * Implements hook_rules_action_info().
 */
function social_media_entity_rules_action_info() {
  // TODO: Implement access control.
  $actions['publish_to_facebook'] = array(
    'label' => t('Publish to Facebook page'),
    'group' => t('Social Media'),
    'parameter' => array(
      'publication' => array(
        'type' => 'facebook_publication',
        'label' => t('Facebook publication'),
      ),
      'pages' => array(
        'type' => 'text',
        'label' => t('Facebook pages'),
        'options list' => 'social_media_entity_available_facebook_pages',
        'access callback' => 'can_administer_facebook_publications',
      ),
    ),
    'base' => 'rules_action_publish_to_facebook',
    'provides' => array(
      'facebook_publication_id' => array(
        'type' => 'text',
        'label' => t('Facebook publication returned ID'),
      ),
    ),
  );
  $actions['publish_to_facebook_timeline'] = $actions['publish_to_facebook'];
  $actions['publish_to_facebook_timeline']['label'] = t("Publish to Facebook user's timeline");
  $actions['publish_to_facebook_timeline']['base'] = 'rules_action_publish_to_facebook_timeline';
  unset($actions['publish_to_facebook_timeline']['parameter']['pages']);
  // Add privacy form to timeline publish.
  $actions['publish_to_facebook_timeline']['parameter']['privacy'] = array(
    'type' => 'text',
    'label' => t('Privacy settings'),
    'options list' => 'social_media_entity_available_privacy_values',
    'access callback' => 'can_administer_facebook_publications',
  );
  $actions['publish_to_facebook_timeline']['parameter']['retry'] = array(
    'type' => 'boolean',
    'label' => t('Retry publication'),
    'description' => t('Users without a valid token to publish in the moment will be redirected to Facebook to authorize and taken back to finish the Facebook publication.'),
    'default value' => TRUE,
    'access callback' => 'can_administer_facebook_publications',
  );
  $actions['delete_from_facebook'] = array(
    'label' => t('Delete from Facebook'),
    'group' => t('Social Media'),
    'parameter' => array(
      'publication_id' => array(
        'type' => 'facebook_publication',
        'label' => t('Target Facebook publication entity'),
        'description' => t('The ID returned from Facebook when creating the publication. Typically stored in the facebook_id property of your FacebookPublicationEntity.'),
      ),
    ),
    'base' => 'rules_action_delete_from_facebook',
  );
  $actions['edit_in_facebook'] = array(
    'label' => t('Update in Facebook'),
    'group' => t('Social Media'),
    'parameter' => array(
      'publication_id' => array(
        'type' => 'facebook_publication',
        'label' => t('Target Facebook publication entity'),
        'description' => t('The ID returned from Facebook when creating the publication. Typically stored in the facebook_id property of your FacebookPublicationEntity.'),
      ),
      'privacy' => array(
        'type' => 'text',
        'label' => t('Privacy settings'),
        'options list' => 'social_media_entity_available_privacy_values',
        'access callback' => 'can_administer_facebook_publications',
        'description' => 'Privacy only applies for publications on timelines',
      ),
      'retry' => array(
        'type' => 'boolean',
        'label' => t('Retry publication'),
        'description' => t('Users without a valid token to publish in the moment will be redirected to Facebook to authorize and taken back to finish the Facebook publication.'),
        'default value' => TRUE,
        'access callback' => 'can_administer_facebook_publications',
      ),
      'destination' => array(
        'type' => 'text',
        'label' => t('Facebook destinations'),
        'options list' => 'available_facebook_destinations',
        'access callback' => 'can_administer_facebook_publications',
        'description' => t('The destination must match the original destination the publication was first published.'),
      ),
    ),
    'base' => 'rules_action_edit_in_facebook',
    'provides' => array(
      'facebook_publication_id' => array(
        'type' => 'text',
        'label' => t('Facebook publication returned ID'),
      ),
    ),
  );

  return $actions;
}

/**
 * Implements hook_rules_conditions_info().
 */
function social_media_entity_rules_condition_info() {
  $conditions['social_media_entity_rules_condition_valid_access_token'] = array(
    'group' => t('Social Media'),
    'label' => t('Facebook access token presence'),
    'parameter' => array(
      'valid_access_token' => array(
        'type' => 'boolean',
        'default value' => TRUE,
        'label' => t('Has a valid access token'),
        'description' => t("If checked, this condition will pass if the current user holds an access token for Facebook. This is useful to detect users that performed a previous action regarding Facebook on the site (ie: Facebook connect). If the user does not have a valid access token one may be granted if the retry checkbox, in the publishing rule, is enabled. The access token is only needed to publish to the user's timeline."),
      ),
    ),
  );
  return $conditions;
}

/**
 * Access callback for permission to 'administer facebook publications'.
 */
function can_administer_facebook_publications() {
  return user_access('administer facebook publications');
}

/**
 * Returns the available options for the $page paramenter.
 */
function social_media_entity_available_facebook_pages() {
  module_load_include('inc', 'social_media', 'social_media.admin');
  $available = array_values(array_filter(variable_get('social_media_page', array())));
  try {
    $fb = new SocialMediaFB();
    $pages_data = $fb->getPagesData(variable_get('social_media_account_id', 'me'), variable_get('social_media_token', ''));
    $options = _social_media_get_page_options($pages_data);
    $output = array();
    foreach ($options as $option => $label) {
      if (in_array($option, $available)) {
        $output[$option] = filter_xss($label);
      }
    }
    return $output;
  }
  catch (Exception $e) {
    watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Returns the available destinations Pages + 'me'.
 */
function available_facebook_destinations() {
  return array_merge(array(
    'me' => '- ' . t("Facebook logged in user's timeline"),
  ), social_media_entity_available_facebook_pages());
}

/**
 * Callback function called when executing the action.
 */
function rules_action_publish_to_facebook(FacebookPublicationEntity $publication, $pages) {
  try {
    $fb = social_media_entity($publication->type);
    $facebook_returned_id = $fb->setDestination($pages)->publish($publication);
    return array('facebook_publication_id' => $facebook_returned_id['id']);
  }
  catch (Exception $e) {
    watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Callback function called when executing the action.
 */
function rules_action_publish_to_facebook_timeline(FacebookPublicationEntity $publication, $privacy, $retry) {
  try {
    $fb = social_media_entity($publication->type);
    $facebook_returned_id = $fb->setDestination('me')
        ->setPrivacy($privacy)
        ->setRetry($retry)
        ->publish($publication);
    return array('facebook_publication_id' => $facebook_returned_id['id']);
  }
  catch (Exception $e) {
    watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Deletes a publication from Facebook.
 */
function rules_action_delete_from_facebook(FacebookPublicationEntity $publication) {
  try {
    $fb = social_media_entity($publication->type);
    $fb->remoteDelete($publication);
  }
  catch (Exception $e) {
    watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Edits a publication in Facebook.
 */
function rules_action_edit_in_facebook(FacebookPublicationEntity $publication, $privacy, $retry, $destination) {
  try {
    $fb = social_media_entity($publication->type);
    if ($destination == 'me') {
      $fb
        ->setPrivacy($privacy)
        ->setRetry($retry);
    }
    $facebook_returned_id = $fb
      ->setDestination($destination)
      ->remoteEdit($publication);
    return array('facebook_publication_id' => $facebook_returned_id['id']);
  }
  catch (Exception $e) {
    watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
    drupal_set_message($e->getMessage(), 'error');
  }
}

/**
 * Callback for valid access token condition.
 */
function social_media_entity_rules_condition_valid_access_token($valid_access_token) {
  return social_media()->hasValidAccessToken() == $valid_access_token;
}

/**
 * Provides a list of privacy settings.
 * 
 * @return array
 *   Containing the options of a select.
 */
function social_media_entity_available_privacy_values() {
  return array(
    'EVERYONE' => t('Public'),
    'FRIENDS_OF_FRIENDS' => t('Friends of friends'),
    'ALL_FRIENDS' => t('All friends'),
    'SELF' => t('Private (only visible to the owner)'),
  );
}
