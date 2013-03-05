<?php

/**
 * @file
 * Class implementation for FBAutopostPhoto
 */

/**
 * Special case for FacebookPublicationType Photo
 */
class SocialMediaFBPhoto extends SocialMediaFB {
  /**
   * Prepares the parameters to publish to Facebook, this means settings any
   * field or destination dependent configuration.
   */
  protected function publishParameterPrepare(&$publication, $destination) {
    parent::publishParameterPrepare($publication, $destination);
    // Add facebook support for uploading files
    $this->setFileUploadSupport(true);
    // Add @ in front of upload URL
    if (!empty($publication['params']['source'])) {
      $image = $publication['params']['source'];
      $uri = empty($image['uri']) ? file_load($image['fid'])->uri : $image['uri'];
      $publication['params']['source'] = '@' . drupal_realpath($uri);
      if (empty($publication['params']['name']) && !empty($image['title'])) {
        $publication['params']['name'] = check_plain($image['title']);
      }
    }
  }
}
