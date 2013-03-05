<?php

/**
 * @file
 * Handles the Entity publication.
 */

/**
 * Handles publishing using a Facebook publication entity.
 */
class SocialMediaEntity extends SocialMediaFB {
  /**
   * Publishes content stored in a Facebook publication entity
   *
   * @param FacebookPublicationEntity $publication
   *   The fully loaded Facebook publication entity.
   * 
   * @return string
   *   Facebook id string for the publication. Needed to edit, or delete the
   *   publication.
   * 
   * @throws ErrorException
   * @see SocialMediaFB::publish()
   */
  public function publish(FacebookPublicationEntity $publication) {
    // Invoke FBAutopost::publish() with the parameters from $publication.
    return parent::publish(array(
      'type' => $publication->type,
      'params' => social_media_entity_get_properties($publication),
    ));
  }

  /**
   * Deletes a publication from facebook stored in an entity.
   *
   * @param FacebookPublicationEntity $publication
   *   The fully loaded Facebook publication entity.
   * 
   * @return boolean
   *   TRUE if the publication was deleted successfully.
   * 
   * @throws ErrorException
   * @see SocialMediaFB::remoteDelete()
   */
  public function remoteDelete(FacebookPublicationEntity $publication) {
    // Get a wrapper for the entity and extract the remote ID.
    $wrapper = entity_metadata_wrapper('facebook_publication', $publication);
    $remote_id = $wrapper->facebook_id->value();
    // If there is a remote ID in return, then delete the associated
    // publication.
    if (!empty($remote_id)) {
      return parent::remoteDelete($remote_id);
    }
    else {
      throw new ErrorException(t('Remote ID could not be found.'), SocialMediaFB::missing_param, WATCHDOG_ERROR);
    }
  }

  /**
   * Edits a publication in facebook from a stored entity.
   *
   * @param FacebookPublicationEntity $publication
   *   The fully loaded Facebook publication entity
   * 
   * @throws ErrorException
   * @see SocialMediaFB::remoteEdit()
   */
  public function remoteEdit(FacebookPublicationEntity $publication) {
    // In this case, edit means delete + publish. This has the side effect that
    // the ID is not preserved.
    if ($this->remoteDelete($publication)) {
      return $this->publish($publication);
    }
  }
}
