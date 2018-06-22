<?php

/**
 * @file
 * Contains \shareContext.
 */

use Drupal\DrupalExtension\Context\DrupalSubContextBase;
use Drupal\DrupalExtension\Context\DrupalSubContextInterface;

/**
 * Context for sharing related custom behat steps.
 */
class shareContext extends DrupalSubContextBase implements DrupalSubContextInterface {

  /**
   * @param $share_id
   *
   * The $share_id is the 'share_id' field from the share_field_data table
   *
   * @Then I set the expiration timestamp to have passed for share ID :share_id
   */

  public function setShareExpirationTimestampToExpired($share_id) {
    $db = \Drupal::database();
    $etm = \Drupal::entityTypeManager();
    /* @var \Drupal\a26_share\Entity\Share */
    $share = $etm->getStorage('share')->load($share_id);
    // Only need to get an older changed date since setting the expiration time
    // is based on the offset from that.
    $todays_date = time();
    $changed_date = strtotime('-20 days', $todays_date);

    $share->setChangedTime($changed_date);
    $share->setExpirationTime();
    $share->save();
  }

  /**
   * @param $share_id
   *
   * The $share_id is the 'share_id' field from the share_field_data table
   *
   * @Then I set the expiration to :timestamp for share ID :share_id
   */

  public function setShareExpirationTimestamp($timestamp, $share_id) {
    $etm = \Drupal::entityTypeManager();
    $share = $etm->getStorage('share')->load($share_id);
    $changed_date = $timestamp;

    $share->setChangedTime($changed_date);
    $share->setExpirationTime();
    $share->save();
  }

}
