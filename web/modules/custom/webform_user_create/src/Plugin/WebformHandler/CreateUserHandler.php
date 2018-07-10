<?php

namespace Drupal\webform_user_create\Plugin\WebformHandler;

use Drupal\user\Entity\User;
use Drupal\webform\Plugin\WebformHandlerBase;
use Drupal\webform\WebformSubmissionInterface;

/**
 * Create a new user entity from a webform submission.
 *
 * @WebformHandler(
 *   id = "create_user",
 *   label = @Translation("Create User"),
 *   category = @Translation("Entity creation"),
 *   description = @Translation("Creates a new drupal user."),
 *   cardinality = \Drupal\webform\Plugin\WebformHandlerInterface::CARDINALITY_UNLIMITED,
 *   results = \Drupal\webform\Plugin\WebformHandlerInterface::RESULTS_PROCESSED,
 * )
 */
class CreateUserHandler extends WebformHandlerBase
{
    /**
     * {@inheritdoc}
     */
    public function postSave(
        WebformSubmissionInterface $webformSubmission,
        $update = true
    ) {
        $user = $this->createUser($webformSubmission);
        // do something with $user
    }

    /**
     * Create user.
     *
     * @param WebformSubmissionInterface $webformSubmission
     *
     * @return User
     */
    protected function createUser(WebformSubmissionInterface $webformSubmission)
    {
        $email = $webformSubmission->getElementData('email');

        /** @var User $user */
        $user = User::create();

        // madatory fields
        $user->setPassword($webformSubmission->getElementData('password'));
        $user->setEmail($email);
        $user->setUsername($webformSubmission->getElementData('username'));
        $user->addRole('subscription_provider');
        $user->enforceIsNew();

        // optional fields
        $user->set('init', $email);
        $user->set('langcode', 'en');
        $user->set('preferred_langcode', 'en');
        $user->set('preferred_admin_langcode', 'en');

        $user->save();

        return $user;
    }
}
