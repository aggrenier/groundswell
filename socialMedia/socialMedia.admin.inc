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
