<?php
/**
* @file
* Administration page callbacks for the annotate module.
*/
/**
* Form builder. Configure annotations.
*
* @ingroup forms
* @see system_settings_form().
*/
function twitter_admin_settings() {
  // Get an array of node types with internal names as keys and
  // "friendly names" as values. E.g.,
  // array('page' => ’Basic Page, 'article' => 'Articles')
  $types = node_type_get_types();
  foreach($types as $node_type) {
    $options[$node_type->type] = $node_type->name;
  }
  
  $form['twitter_consumer_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Consumer Key'),
    '#description' => t('Enter your consumer Key'),
    '#default_value' => variable_get('twitter_consumer_key', 1),
    '#size' => 20
  ); 
  $form['twitter_consumer_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Consumer Secret'),
    '#description' => t('Enter the consumer Secret'),
    '#default_value' => variable_get('twitter_consumer_secret', 1),
    '#size' => 20
  ); 
  $form['twitter_oauth_token'] = array(
    '#type' => 'textfield',
    '#title' => t('OAuth Token'),
    '#description' => t('Enter your OAuth Token'),
    '#default_value' => variable_get('twitter_oauth_token', 1),
    '#size' => 20
  ); 
  $form['twitter_oauth_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('OAuth Secret'),
    '#description' => t('Enter your OAuth secret'),
    '#default_value' => variable_get('twitter_oauth_secret', 1),
    '#size' => 20
  );
  
  $form['#submit'][] = 'social_media_admin_settings_submit';
  return system_settings_form($form);
}

function social_media_global_settings() {
  // Data for the Facebook App connection.
  $app_id = variable_get('social_media_app_id', '');
  $secret = variable_get('social_media_app_secret', '');
  $form['app_data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Facebook App data'),
    '#collapsible' => TRUE,
    // Collapse it if all data is in place.
    '#collapsed' => !empty($app_id) && !empty($secret),
  );
  $form['app_data']['social_media_app_id'] = array(
    '#type' => 'textfield',
    '#title' => t('APP ID'),
    '#description' => t('The API key of your Facebook App.'),
    '#default_value' => $app_id,
    '#required' => TRUE,
  );
  $form['app_data']['social_media_app_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('APP Secret'),
    '#description' => t('The API secret of your Facebook App.'),
    '#default_value' => $secret,
    '#required' => TRUE,
  );

  // If we have all the info we need, show next part of the form.
  if (!empty($app_id) && !empty($secret)) {
    $form['page_info'] = array(
      '#type' => 'fieldset',
      '#title' => t('Facebook page information'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    try {
      // Create the FBAutpost object with the stored data.
      $fb = new SocialMediaFB();
      if ($fb_user = $fb->getUser()) {
        // If there is a connected account, get all FB pages related to that
        // account. Make these pages the values of a select.
        try {
          // Proceed knowing you have a logged in user who's authenticated.
          $form['page_info']['fb_user'] = array(
            '#theme' => 'social_media_facebook_profile',
            '#fbprofile_id' => variable_get('social_media_account_id', 'me'),
          );
          $pages_data = $fb->getPagesData(variable_get('social_media_account_id', 'me'), variable_get('social_media_token', ''));
          $form['page_info']['social_media_page'] = array(
            '#type' => 'checkboxes',
            '#title' => t('Pages'),
            '#description' => t('Select the Facebook page you want to post to.'),
            '#options' => _social_media_get_page_options($pages_data),
            '#default_value' => variable_get('social_media_page', array()),
          );
          $form['page_info']['social_media_pages_access_tokens'] = array(
            '#tree' => TRUE,
          );
          foreach ($pages_data['data'] as $page_data) {
            $form['page_info']['social_media_pages_access_tokens'][$page_data['id']] = array(
              '#type' => 'hidden',
              '#value' => $page_data['access_token'],
            );
          }
        }
        catch (FacebookApiException $e) {
          watchdog('social_media', $e->getMessage(), array(), WATCHDOG_ERROR);
          drupal_set_message($e->getMessage(), 'error');
          $user = NULL;
        }
      }
      else {
        // Invite the user to log in.
        $form['page_info']['login_link'] = array(
          '#prefix' => '<p>' . t('You need to connect your Facebook account to fetch the pages you are allowed to post to.') . '</p>',
          '#type' => 'link',
          '#title' => t('Login to Facebook'),
          '#href' => $fb->getLoginUrl(array(
            'scope' => 'publish_stream,manage_pages',
            'redirect_uri' => url('admin/config/socialMedia/login', array('absolute' => TRUE)),
          )),
        );
      }
    }
    catch (Exception $e) {
      // Do nothing with the exception.
      drupal_set_message($e->getMessage(), 'error');
    }
  }
  $form['#submit'][] = 'social_media_global_settings_submit';
  return system_settings_form($form);
}

/**
 * Submit callback for social_media_global_settings.
 */
function social_media_global_settings_submit($form, &$form_state) {
  if (isset($form_state['values']['social_media_page'])) {
    $pages = array_values(array_filter($form_state['values']['social_media_page']));
    $at = $form_state['values']['social_media_pages_access_tokens'];
    $form_state['values']['social_media_pages_access_tokens'] = array();
    foreach ($pages as $page_id) {
      $form_state['values']['social_media_pages_access_tokens'][$page_id] = $at[$page_id];
    }
  }
}

/**
 * Helper function to get all the available pages.
 * 
 * @param array $pages_data
 *   An array containig the Facebook information about the page as returned
 *   from SocialMedia::getPagesData(…).
 * 
 * @return array
 *   Array containing an options friendly array with all writable pages
 */
function _social_media_get_page_options($pages_data) {
  $output = array();
  $data = array_filter($pages_data['data'], function ($item) {
    return $item['category'] != 'Application' && in_array('CREATE_CONTENT', $item['perms']);
  });
  foreach ($data as $value) {
    $output[$value['id']] = theme('social_media_facebook_page', array(
      'name' => $value['name'],
      'id' => $value['id'],
      'category' => $value['category'],
    ));
  }
  return $output;
}


