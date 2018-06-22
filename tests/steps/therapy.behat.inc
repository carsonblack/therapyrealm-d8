<?php

/**
 * @file
 * Contains \A26SubContext.
 */

use Behat\Behat\Hook\Scope\BeforeScenarioScope;
use Behat\Gherkin\Node\PyStringNode;
use Behat\Mink\Exception\ElementNotFoundException;
use Behat\Mink\Exception\ExpectationException;
use Behat\Mink\Exception\UnsupportedDriverActionException;
use Drupal\DrupalExtension\Context\DrupalSubContextBase;
use Drupal\DrupalExtension\Context\DrupalSubContextInterface;
use Drupal\node\Entity\Node;

/**
 * Sub context for TherapyRealm step definitions.
 */
class TherapySubContext extends DrupalSubContextBase implements DrupalSubContextInterface {

  /**
   * The Mink context.
   *
   * @var \Drupal\DrupalExtension\Context\MinkContext
   */
  protected $minkContext;

  /**
   * Contains the name of the currently selected iframe.
   *
   * @var string
   */
  private $iframe = NULL;

  /**
   * The filename of a file which has been uploaded.
   *
   * @var null
   */
  protected $currentFileName = NULL;

  /**
   * Pre-scenario hook.
   *
   * @BeforeScenario
   */
  public function gatherContexts(BeforeScenarioScope $scope) {
    $environment = $scope->getEnvironment();

    $this->minkContext = $environment->getContext('Drupal\DrupalExtension\Context\MinkContext');
  }

  /**
   * Get a region by name.
   *
   * @param string $region
   *   The name of the region from the behat.yml file.
   *
   * @return Behat\Mink\Element\Element
   *   An element representing the region.
   */
  public function getRegion($region) {
    $session = $this->getSession();
    $regionObj = $session->getPage()->find('region', $region);
    if (!$regionObj) {
      throw new \Exception(sprintf('No region "%s" found on the page %s.', $region, $session->getCurrentUrl()));
    }
    return $regionObj;
  }

  /**
   * Asserts that an element, specified by CSS selector, exists.
   *
   * @param string $selector
   *   The CSS selector to search for.
   *
   * @Then the element :selector should exist
   */
  public function theElementShouldExist($selector) {
    $this->assertSession()->elementExists('css', $selector);
  }

  /**
   * Clicks an arbitrary element, found by CSS selector.
   *
   * @param string $selector
   *   The CSS selector.
   *
   * @throws \Behat\Mink\Exception\ElementNotFoundException
   *   If the specified element is not present on the page.
   *
   * @When I click the :selector element
   */
  public function clickElementBySelector($selector) {
    $session = $this->getSession();

    $element = $session->getPage()->find('css', $selector);
    if ($element) {
      try {
        $this->scrollToElement($selector);
      }
      catch (UnsupportedDriverActionException $e) {
        // Don't worry about it.
      }
      $element->click();
    }
    else {
      throw new ElementNotFoundException($session, 'element', 'css', $selector);
    }
  }

  /**
   * Asserts that a role as a set of permissions.
   *
   * @param string $rid
   *   The role ID.
   * @param \Behat\Gherkin\Node\PyStringNode $permissions
   *   The permissions to check for.
   *
   * @Then the :role role should have permissions:
   * @Then the :role role should have permission to:
   */
  public function assertPermissions($rid, PyStringNode $permissions) {
    foreach ($permissions->getStrings() as $permission) {
      $this->assertPermission($rid, $permission);
    }
  }

  /**
   * Asserts that a role does NOT have a set of permissions.
   *
   * @param string $rid
   *   The role ID.
   * @param \Behat\Gherkin\Node\PyStringNode $permissions
   *   The permissions to check for.
   *
   * @Then the :role role should not have permissions:
   * @Then the :role role should not have permission to:
   */
  public function assertNoPermissions($rid, PyStringNode $permissions) {
    foreach ($permissions->getStrings() as $permission) {
      $this->assertNoPermission($rid, $permission);
    }
  }

  /**
   * Asserts that a role has a specific permission.
   *
   * @param string $rid
   *   The role ID.
   * @param string $permission
   *   The permission to check for.
   *
   * @Given the :role role has the :permission permission
   * @Given the :role role has permission to :permission
   *
   * @Then the :role role should have the :permission permission
   * @Then the :role role should have permission to :permission
   */
  public function assertPermission($rid, $permission) {
    $this->minkContext->assertAtPath('/admin/people/permissions/' . $rid);
    $this->assertSession()->checkboxChecked($rid . '[' . $permission . ']');
  }

  /**
   * Asserts that a role does not have a specific permission.
   *
   * @param string $rid
   *   The role ID.
   * @param string $permission
   *   The permission to check for.
   *
   * @Given the :role role does not have the :permission permission
   * @Given the :role role does not have permission to :permission
   *
   * @Then the :role role should not have the :permission permission
   * @Then the :role role should not have permission to :permission
   */
  public function assertNoPermission($rid, $permission) {
    $this->minkContext->assertAtPath('/admin/people/permissions/' . $rid);

    $field = $rid . '[' . $permission . ']';
    try {
      $this->assertSession()->fieldNotExists($field);
    }
    catch (ExpectationException $e) {
      $this->assertSession()->checkboxNotChecked($field);
    }
  }

  /**
   * Gives a username a specific role.
   *
   * @Given I give the user :username the role :rolename
   */
  public function iGiveUsernameTheRole($username, $rolename) {
    $account = user_load_by_name($username);
    $account->addRole($rolename);
    $account->save();
  }

  /**
   * Creates a video entity from an embed code.
   *
   * @param \Behat\Gherkin\Node\PyStringNode $embed_code
   *   The embed code.
   *
   * @Given video from embed code:
   *
   * @When I create a video from embed code:
   */
  public function videoFromEmbedCode(PyStringNode $embed_code) {
    $this->createMediaFromEmbedCode('video', $embed_code, 'field_media_video_embed_field');
  }

  /**
   * Creates a media asset from an embed code.
   *
   * @param string $bundle
   *   The media bundle ID.
   * @param \Behat\Gherkin\Node\PyStringNode $embed_code
   *   The embed code.
   * @param string $field
   *   (optional) The embed code field. Defaults to embed_code.
   *
   * @Given :bundle media from embed code:
   *
   * @When I create :bundle media from embed code:
   */
  public function createMediaFromEmbedCode($bundle, PyStringNode $embed_code, $field = 'embed_code') {
    $entity = \Drupal::entityTypeManager()
      ->getStorage('media')
      ->create([
        'bundle' => $bundle,
        'name' => $this->getRandom()->string(),
        $field => (string) $embed_code,
        'field_media_in_library' => TRUE,
        'status' => TRUE,
      ]);
    $entity->save();

    /** @var \EntitySubContext $context */
    $context = $this->getContext('EntitySubContext');
    $context->queueEntity($entity);
  }

  /**
   * Scrolls an element into the viewport.
   *
   * @param string $selector
   *   The element's CSS selector.
   *
   * @When I scroll to the :selector element
   */
  public function scrollToElement($selector) {
    $this->getSession()
      ->executeScript('document.querySelector("' . addslashes($selector) . '").scrollIntoView()');
  }

  /**
   * Expands a drop button.
   *
   * @param string $selector
   *   The data-drupal-selector attribute of the drop button.
   *
   * @When I expand the :selector drop button
   */
  public function expandDropButton($selector) {
    $selector = 'ul.dropbutton[data-drupal-selector="' . $selector . '"]';
    $this->clickElementBySelector($selector . ' li.dropbutton-toggle');
    $this->scrollToElement($selector);
  }

  /**
   * Maximizes the window (probably for debugging purposes).
   *
   * @When I maximize the window
   */
  public function maximize() {
    $this->getSession()->getDriver()->maximizeWindow();
  }

  /**
   * Takes a screenshot for debugging purposes.
   *
   * @param string $filename
   *   The name of the screenshot file.
   *
   * @When I take a screenshot named :filename
   */
  public function takeScreenshot($filename) {
    $screenshot = $this->getSession()->getDriver()->getScreenshot();
    // If this file is in tests/features/bootstrap, the screenshot be in tests.
    file_put_contents('' . $filename . '.png', $screenshot);
  }

  /**
   * Fills in an autocompleting entity reference field.
   *
   * @param string $entity_type
   *   The target entity type.
   * @param string $label
   *   The target entity's label.
   * @param string $field
   *   Entity reference field locator.
   *
   * @throws \Exception
   *   If no entities of the given type, with the given label, are found.
   *
   * @When I reference :entity_type :label in :field
   */
  public function referenceEntityByLabel($entity_type, $label, $field) {
    $label_property = \Drupal::entityTypeManager()
      ->getDefinition($entity_type)
      ->getKey('label');

    $entities = \Drupal::entityTypeManager()
      ->getStorage($entity_type)
      ->loadByProperties([
        $label_property => $label,
      ]);

    if ($entities) {
      $this->minkContext->fillField($field, $label . ' (' . reset($entities)->id() . ')');
    }
    else {
      throw new \Exception("No $entity_type labeled '$label' was found.");
    }
  }

  /**
   * Asserts that an image is present and not broken.
   *
   * @Then I should see an image in the :region( region)
   */
  public function assertValidImageRegion($region) {
    $regionObj = $this->getRegion($region);
    $elements = $regionObj->findAll('css', 'img');
    if (empty($elements)) {
      throw new \Exception(sprintf('No image was not found in the "%s" region on the page %s', $region, $this->getSession()->getCurrentUrl()));
    }

    if ($src = $elements[0]->getAttribute('src')) {
      // Make an absolute URL if a relative one is provided
      if (preg_match('/^\//', $src)) {
        global $base_url;
        global $base_path;
        $src = $base_url . $base_path . $src;
      }

      $params = array('http' => array('method' => 'HEAD'));
      $context = stream_context_create($params);
      $fp = @fopen($src, 'rb', FALSE, $context);
      if (!$fp) {
        throw new \Exception(sprintf('Unable to download <img src="%s"> in the "%s" region on the page %s', $src, $region, $this->getSession()->getCurrentUrl()));
      }

      $meta = stream_get_meta_data($fp);
      fclose($fp);
      if ($meta === FALSE) {
        throw new \Exception(sprintf('Error reading from <img src="%s"> in the "%s" region on the page %s', $src, $region, $this->getSession()->getCurrentUrl()));
      }

      $wrapper_data = $meta['wrapper_data'];
      $found = FALSE;
      if (is_array($wrapper_data)) {
        foreach ($wrapper_data as $header) {
          if (substr(strtolower($header), 0, 19) == 'content-type: image') {
            $found = TRUE;
          }
        }
      }

      if (!$found) {
        throw new \Exception(sprintf('Not a valid image <img src="%s"> in the "%s" region on the page %s', $src, $region, $this->getSession()->getCurrentUrl()));
      }
    }
    else {
      throw new \Exception(sprintf('No image had no src="..." attribute in the "%s" region on the page %s', $region, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then I should see the image alt :text in the :region( region)
   *
   * NOTE: We specify a regex to allow escaped quotes in the alt text.
   */
  public function assertAltRegion($text, $region) {
    $regionObj = $this->getRegion($region);
    $element = $regionObj->find('css', 'img');
    $tmp = $element->getAttribute('alt');
    if ($text == $tmp) {
      $result = $text;
    }
    if (empty($result)) {
      throw new \Exception(sprintf('No alt text matching "%s" in the "%s" region on the page %s', $text, $region, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @Then the :field select should be set to :value
   */
  public function theSelectShouldBeSetTo($field, $value) {
    $select = $this->getSession()->getPage()->findField($field);
    if (empty($select)) {
      throw new \Exception(sprintf('We couldn\'nt find "%s" on the page', $field));
    }
    $options = $select->findAll('xpath', '//option[@selected="selected"]');
    if (empty($select)) {
      throw new \Exception(sprintf('The select "%s" doesn\'t have any options selected', $field));
    }

    $found = FALSE;
    foreach ($options as $option) {
      if ($option->getText() === $value) {
        $found = TRUE;
        break;
      }
    }
    if (!$found) {
      throw new \Exception(sprintf('The select "%s" doesn\'t have the option "%s" selected', $field, $value));
    }
  }

  /**
   * @Then the :field autocomplete field should contain :text
   *
   * Checks, that autocomplete form field with specified id|name|label|value has specified value.
   */
  public function theAutocompleteFieldShouldContain($field, $text)
  {
    $node = $this->assertSession()->fieldExists($field);
    $actual = $node->getValue();

    // Only look for the text at the beginning of the actual value since the
    // actual value will have a node ID appended which we won't know the value of
    if (strpos($text, ',') === FALSE) {
      $regex = '/' . preg_quote($text, '/') . '\s+\(\d+\)/ui';
    }
    else {
      // append the term id regex to the end of each term
      $partials = explode(',', $text);

      foreach ($partials as $key => $partial_text) {
        $partials[$key] = preg_quote(trim($partial_text), '/') . '\s+\(\d+\)';
      }

      $regex = '/' . implode(', ', $partials) . '/ui';
    }

    if (!preg_match($regex, $actual)) {
      throw new \Exception(sprintf('The field "%s" value is "%s", but "%s" expected.', $field, $actual, $text));
    }
  }

  /**
   * @Given the Administration Toolbar is hidden
   *
   * We sometimes need to hide the admin toolbar so that buttons can be clicked
   */
  public function theAdministrationToolbarIsHidden()
  {
    $this->getSession()->executeScript('jQuery("#toolbar-administration").hide();');
  }

  /**
   * @Then I( should) see :text in the :tag element in the :region( region)
   */
  public function iShouldSeeTextInTheElementInTheRegion($text, $tag, $region) {
    $regionObj = $this->getRegion($region);
    $elements = $regionObj->findAll('css', $tag);
    if (empty($elements)) {
      throw new \Exception(sprintf('The element "%s" was not found in the "%s" region on the page %s', $tag, $region, $this->getSession()->getCurrentUrl()));
    }

    $found = FALSE;
    foreach ($elements as $element) {
      if ($element->getText() == $text) {
        $found = TRUE;
        break;
      }
    }
    if (!$found) {
      throw new \Exception(sprintf('The text "%s" was not found in the "%s" element in the "%s" region on the page %s', $text, $tag, $region, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * @When I switch to the frame :frame
   */
  public function iSwitchToTheFrame($frame) {
    $this->getSession()->switchToIFrame($frame);
    $this->iframe = $frame;
  }

  /**
   * @When I switch to the frame with :element element
   */
  public function iSwitchToTheFrameWithElement($element) {
    // Get the frame ID from the given element
    $container = $this->getSession()->getPage();
    $node = $container->find('css', $element);

    if (null === $node) {
      if (is_array($element)) {
        $element = implode(' ', $element);
      }

      throw new ElementNotFoundException($this->getSession()->getDriver(), 'element', 'css', $element);
    }

    $frame = $node->getAttribute('id');

    $this->getSession()->switchToIFrame($frame);
    $this->iframe = $frame;
  }

  /**
   * @When I switch out of all frames
   */
  public function iSwitchOutOfAllFrames() {
    $this->getSession()->switchToIFrame();
    $this->iframe = NULL;
  }

  /**
   * Sets the updated and created date of a piece of content to the given time
   * frame.
   *
   * @param string $time_ago
   *  How long ago. See PHP's strtotime function:
   *  http://php.net/manual/en/function.strtotime.php
   *
   * * @param string $title
   *  The title of the node to set the updated date for.
   *
   * @Given I set the last updated date to :time_ago for the node with the title :title
   */
  public function setUpdatedDate($time_ago, $title) {
    // get the last node added with the given title.
    $node_select = \Drupal::database()->select('node_field_data', 'n')
      ->fields('n', ['nid'])
      ->range(0, 1)
      ->orderBy('n.nid', 'DESC');
    $node_select->condition('n.title', $title);
    $node_id = $node_select->execute()->fetchField();
    // Make sure we found our node.
    if(!is_numeric($node_id) || is_null($node_id)) {
      throw new \Exception(sprintf('The node "%s" was not found.', $node_id));
    }

    // Load the node and set the created and changed date to given time ago.
    // Set the created and changed date to the same date for some consistency.
    $node = Node::load($node_id);
    $node->set('changed',strtotime($time_ago));
    $node->set('changed',strtotime($time_ago));
    $node->save();
  }

  /**
   * Clicks a link in the first dropbutton
   *
   * @param string $text
   *   The id|title|alt|text link.
   *
   * @Then I click the :text link that is in the first dropbutton
   */
  public function iClickTheLinkThatIsInTheFirstDropbutton ($text) {
    $page = $this->getSession()->getPage();
    // Get the containing element by it's css selector
    $container = $page->find('css','ul.dropbutton');
    if (empty($container)) {
      throw new \Exception(sprintf("The %s was not found on this page.",$selector));
    }
    // Get the link inside the containing element
    $link = $container->findLink($text);
    if (empty($link)) {
      throw new \Exception(sprintf("There is no link wiht the text: %s",$text));
    }
    $link->click();
  }

  /**
   * Clicks a link in the Main Menu even if hidden in mobile.
   *
   * @param string $link
   *   The id|title|alt|text link.
   *
   * @When I click :link in the main menu
   */
  public function clickLinkInMainMenu($link) {
    $session = $this->getSession();
    $region = 'Navigation Region';
    // Check if the mobile nav toggle is visible
    $toggle = $session->getPage()->find('css', '.nav-toggle');
    if ($toggle->isVisible()) {
      // Toggle open the main menu
      $toggle->click();
    }
    // Click the menu link
    $this->minkContext->assertRegionLinkFollow($link, $region);
  }

  /**
   * Check to see if something is shown before something else in a given region.
   * Useful for checking if the sorting of a view is working.
   *
   * @param string $arg1
   *  The text string I should see before arg2
   *
   * @param string $arg2
   *  The text string I should see after arg1
   *
   * @param string $region
   *  The region name
   *
   * @Then I should see :arg1 before :arg2 in the :region region
   */
  public function iShouldSeeBeforeInTheRegion($arg1, $arg2, $region)
  {
    //Get the text from the region
    $regionObj = $this->getRegion($region);
    $text = $regionObj->getText();
    // get the strong positions of each of the strings given
    $strpos_first = strpos($text,$arg1);
    $strpos_second = strpos($text,$arg2);
    // If the first string is in a higher number position than the second one
    // they are in the wrong order.
    if ($strpos_first > $strpos_second) {
      throw new \Exception(sprintf("%s is not being shown before %s in the %s region",$arg1,$arg2,$region));
    }

  }

  /**
   * @Given I capture the filename for :link
   *
   * Get the filename for a link and store it for later use.
   */
  public function iCaptureFilename($link) {
    $element = $this->getSession()->getPage();
    /** @var \Behat\Mink\Element\NodeElement $result */
    $result = $element->findLink($link);

    try {
      if ($result && !$result->isVisible()) {
        throw new \Exception(sprintf("No link to '%s' on the page %s", $link, $this->getSession()->getCurrentUrl()));
      }

      if ($result) {
        $this->currentFileName = $result->getAttribute('href');
      }
    }
    catch (UnsupportedDriverActionException $e) {
      // We catch the UnsupportedDriverActionException exception in case
      // this step is not being performed by a driver that supports javascript.
      // All other exceptions are valid.
    }

    if (empty($result)) {
      throw new \Exception(sprintf("No link to '%s' on the page %s", $link, $this->getSession()->getCurrentUrl()));
    }
  }

  /**
   * Opens specified page with the current filename appended to it.
   *
   * @Given I am on :page with the current filename appended
   */
  public function onPageWithCurrentFilenameAppended($page) {
    $this->visitPath($page . $this->currentFileName);
  }

}
