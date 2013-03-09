<?php

/**
 * @file
 * Handles session management for the Social Media module
 */

/**
 * Class implementation
 */
class SocialMediaFBSession {

  /**
   * Private method to store the required publication info to get it back later
   * when returning from Facebook authorization
   *
   * @param $publication
   *   The publication array
   * @param $destination
   *   The URI the user will be redirected after the publication
   */
  public function storePublication($publication, $destination = NULL) {
    $_SESSION['social_media_authorization_required'] = array(
      'publication' => $publication,
      'target' => 'me',
    );
    if ($destination) {
      $_SESSION['social_media_authorization_required']['destination'] = $destination;
    }
    else {
      $_SESSION['social_media_authorization_required'] += drupal_get_destination();
    }
  }

  /**
   * Determines if there is information stored in the session about the
   * publication
   */
  public function isStored() {
    return isset($_SESSION['social_media_authorization_required']);
  }

  /**
   * Get the stored data in the session
   * @return
   *   The data stored via FBAutopost::storeSessionPublication()
   * @see SocialMediaFB::storeSessionPublication()
   */
  public function getStoredPublication() {
    return $_SESSION['social_media_authorization_required'];
  }

  /**
   * Remove the stored data in the session. This is important to know if the
   * user is trying to be authorized.
   */
  public function removePublication() {
    if (isset($_SESSION)) {
      unset($_SESSION['social_media_authorization_required']);
    }
  }
}
