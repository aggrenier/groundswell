<?php

/**
 * @file
 * Handles all FB integration to Authorize and to Post
 */

_load_facebook_sdk();

/**
 * API class to handle common actions when autoposting
 * This class uses ErrorException for error handling. Severity is
 * passed resusing watchdog severity (See: http://api.drupal.org/api/drupal/includes%21bootstrap.inc/function/watchdog/7)
 * @see https://developers.facebook.com/docs/howtos/login/server-side-login
 */
class FBAutopost extends Facebook {
  /**
   * Constant indicating error code for a missing parameter
   */
  const missing_param = 0;
  /**
   * Constant indicating error code for an icorrect parameter
   */
  const incorrect_param = 1;
  /**
   * Constant indicating error code for a missing dependency
   */
  const missing_dependency = 2;
  /**
   * Constant indicating error code for a SDK thrown error
   */
  const sdk_error = 3;

  /**
   * Publication types as defined in Facebook documentation
   * Contains the name of the publication and the endpoint keyed by the machine name of the publication.
   * @see https://developers.facebook.com/docs/reference/api/page
   */
  private $publication_types = array(
    'event' => array(
      'name' => 'Event',
      'endpoint' => 'events',
    ),
    'link' => array(
      'name' => 'Links',
      'endpoint' => 'feed',
    ),
    'note' => array(
      'name' => 'Note',
      'endpoint' => 'notes',
    ),
    'photo' => array(
      'name' => 'Photo',
      'endpoint' => 'photos',
    ),
    'post' => array(
      'name' => 'Post',
      'endpoint' => 'feed',
    ),
    'question' => array(
      'name' => 'Question',
      'endpoint' => 'questions',
    ),
    'milestone' => array(
      'name' => 'Milestone',
      'endpoint' => 'milestones',
    ),
    'offer' => array(
      'name' => 'Offer',
      'endpoint' => 'offers',
    ),
    'message' => array(
      'name' => 'Message',
      'endpoint' => 'conversations',
    ),
    'status' => array(
      'name' => 'Status',
      'endpoint' => 'feed',
    ),
    'video' => array(
      'name' => 'Video',
      'endpoint' => 'videos',
    ),
    'tab' => array(
      'name' => 'Tab',
      'endpoint' => 'tabs',
    ),
  );

  function __construct($conf = array()) {
    if (!isset($conf['appId'])) {
      $conf['appId'] = variable_get('fb_autopos_app_id', '');
    }
    if (!isset($conf['secret'])) {
      $conf['secret'] = variable_get('fb_autopos_app_secret', '');
    }
    parent::__construct($conf);
  }

  /**
   * Publishes content in the selected pages
   * @param $publication
   *   Keyed array containing the info for the publication. Must contain the keys:
   *     - 'type': The publication type as defined in $publication_types
   *     - 'params': Associative array with the necessary params for a successful FB publication
   * @param $on
   *   Array containig page ids (among those already selected via UI). Use 'all' as a wildcard.
   *   Use 'me' to publish to the user's timeline
   * @return
   *   Facebook id string for the publication. Needed to edit, or delete the publication.
   * @throws ErrorException
   */
  function publish($publication, $on = 'all') {
    // Error generation
    if (!isset($publication['type']) || !isset($publication['params'])) {
      throw new ErrorException(t('The publication array must contain publication type and publication parameters.'), FBAutopost::missing_param, WATCHDOG_ERROR);
    }
    // Check if the publication type is unkown
    if (!in_array($publication['type'], array_keys($this->publication_types))) {
      throw new ErrorException(t('The publication type you specified is invalid.'), FBAutopost::incorrect_param, WATCHDOG_ERROR);
    }
    // Get fresh access tokens for the pages. We use the server side access token and the account ID to retrieve them
    $pages = $this->getPagesAccessTokens(variable_get('fb_autopost_account_id', 'me'), variable_get('fb_autopost_token', ''));

    if ($on == 'all') {
      // Get all available pages.
      $on = array_keys($pages);
    }
    // Do not try to find an stored access token whe publishing on user's timelines
    elseif ($on == 'me') {
      $on = array('me');
    }
    else {
      // If selective pages, then check that the selected pages are in the available list.
      foreach ($on as $page_id) {
        if (!in_array($page_id, array_keys($pages))) {
          throw new ErrorException(t('Unsuficient permissions to publish on page with id %id. Please check !config.', array('%id' => $page_id, '!config' => l(t('your configuration'), 'admin/config/services/fbautopost'))), FBAutopost::incorrect_param, WATCHDOG_ERROR);
        }
      }
    }
    // Now we can start the publication.
    try {
      foreach ($on as $page_id) {
        if ($page_id != 'me') {
          $publication['params'] += array('access_token' => $pages[$page_id]['access_token']);
        }
        $args = array(
          '/' . $page_id . '/' . $this->publication_types[$publication['type']]['endpoint'], // Post to page_id on the selected endpoint
          'POST', // This is fixed
          $publication['params'], // Add access token to the params
        );
        // Call api method from ancestor
        return call_user_func_array(array($this, 'api'), $args);
      }
    } catch (FacebookApiException $e) {
      // If trying to publish on timeline for the first time
      if ($on == 'me' || $on == array('me') && !$this->isStoredSession()) {
        // Here we can handle the unauthorized user error
        // To do so we save the relevant data in $_SESSION, then redirect to
        // Facebook athorization URL. This URL redirects back to a fixed location
        // that handles the publishing and redirects back to the original location
        $this->storeSessionPublication($publication);
        $login_url = $this->getLoginUrl(array(
          'scope' => 'publish_stream',
          'redirect_uri' => url('fbautopost/authorization/retry', array('absolute' => TRUE)),
        ));
        drupal_goto($login_url);
      }
      else {
        // Throw an exception
        throw new ErrorException(t('Facebook SDK threw an error of type %type: %error', array('%error' => $e, '%type' => $e->getType())), FBAutopost::sdk_error, WATCHDOG_ERROR);
      }
    }
  }

  /**
   * Private method to store the required publication info to get it back later
   * when returning from Facebook authorization
   * 
   * @param $publication
   *   The publication array
   * @param $destination
   *   The URI the user will be redirected after the publication
   */
  private function storeSessionPublication($publication, $destination = NULL) {
    $_SESSION['fb_autopost_authorization_required'] = array(
      'publication' => $publication,
      'target' => 'me',
    );
    if ($destination) {
      $_SESSION['fb_autopost_authorization_required']['destination'] = $destination;
    }
    else {
      $_SESSION['fb_autopost_authorization_required'] += drupal_get_destination();
    }
  }

  /**
   * Determines if there is information stored in the session about the publication
   */
  public function isStoredSession() {
    return isset($_SESSION['fb_autopost_authorization_required']);
  }

  /**
   * Get the stored data in the session
   * @return
   *   The data stored via FBAutopost::storeSessionPublication()
   * @see FBAutopost::storeSessionPublication
   */
  function getStoredSessionPublication() {
    return $_SESSION['fb_autopost_authorization_required'];
  }

  /**
   * Remove the stored data in the session. This is important to know if the user
   * is trying to be authorized.
   */
  public function removeSessionPublication() {
    unset($_SESSION['fb_autopost_authorization_required']);
  }

  /**
   * Get access tokens for publishing to several Facebook Pages
   * @param $account_id
   *   User account asked for
   * @param $account_access_token
   *   Server side access token stored from the admin form.
   * @return
   *   A keyed array indexed by page id and containing the values:
   *     - 'id': Facebook page id
   *     - 'access_token': Access token for publishing to the page
   * @throws ErrorException
   */
  private function getPagesAccessTokens($account_id, $account_access_token) {
    $pages = array();
    foreach (variable_get('fb_autopost_pages_access_tokens', array()) as $page_id => $page_access_token) {
      $pages[$page_id] = array(
        'id' => $page_id,
        'access_token' => $page_access_token,
      );
    }
    return $pages;
  }

  /**
   * Gets the reply given from Facebook when asking for user account.
   * @param $account_id
   *   User account asked for
   * @param $account_access_token
   *   Server side access token stored from the admin form.
   * @return
   *   The array from the Graph API call
   * @throws ErrorException
   */
  public function getPagesData($account_id, $account_access_token) {
    // Check if there is an access_token available.
    if (empty($account_access_token)) {
      throw new ErrorException(t('Cannot ask for user accounts without an access token.'), FBAutopost::missing_param, WATCHDOG_ERROR);
    }
    try {
      return $this->api('/' . $account_id . '/accounts', 'GET', array(
        'limit' => 999,
      ));

    } catch (FacebookApiException $e) {
      // Get the FacebookApiException and throw an ordinary ErrorException
      throw new ErrorException(t('Facebook SDK threw an error of type %type: %error', array('%error' => $e, '%type' => $e->getType())), FBAutopost::sdk_error, WATCHDOG_ERROR);
    }
  }

  /**
   * Publishes content stored in a Facebook publication entity to a Facebook page.
   * @param $publication
   *   The fully loaded Facebook publication entity
   * @param $on
   *   Array containig page ids (among those already selected via UI). Use 'all' as a wildcard.
   * @return
   *   Facebook id string for the publication. Needed to edit, or delete the publication.
   * @throws ErrorException
   * @see FBAutopost::publish
   */
  public function entity_publish(FacebookPublicationEntity $publication, $on = 'all') {
    // Check if fb_autopost_entity is loaded and throw an exception if its not
    if (!module_exists('fb_autopost_entity')) {
      throw new ErrorException(t('You cannot use %method without activating %module.', array('%method' => 'entity_publish', '%module' => 'fb_autopost_entity')), FBAutopost::missing_dependency, WATCHDOG_ERROR);
    }
    // TODO: Documentation
    return $this->publish(array(
      'type' => $publication->type,
      'params' => fb_autopost_entity_get_properties($publication),
    ), $on);
  }
}
