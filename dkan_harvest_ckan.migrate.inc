<?php

/**
 * @file dkan_harvest_ckan.migrate.inc
 *
 * Migration File for DKAN Harvest of CKAN file from
 *    /api/3/action/current_package_list_with_resources.
 */

/**
 * Class HarvestMigrationCKAN.
 *
 * Harvest the CKAN API on /api/3/action/current_package_list_with_resources.
 */
class HarvestMigrationCKAN extends HarvestMigration {

  public $itemUrl;

  /**
   * Constructor.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->itemUrl = drupal_realpath($this->dkanHarvestSource->getCacheDir()) .
            '/:id';

    $this->source = new MigrateSourceList(
            new HarvestList($this->dkanHarvestSource->getCacheDir()),
            new MigrateItemJSON($this->itemUrl),
            $this->getCkanDatasetFields(),
            $this->sourceListOptions
    );
  }

  /**
   * {@inheritdoc}
   */
  public function setFieldMappings($version = '') {
    parent::setFieldMappings();

    $this->addFieldMapping('uuid', 'id');
    $this->addFieldMapping('changed', 'metadata_modified');
    $this->addFieldMapping('created', 'metadata_created');
    $this->addFieldMapping('field_contact_email', 'maintainer_email');
    $this->addFieldMapping('field_license', 'license_title');
    $this->addFieldMapping('field_tags', '_tags');
    $this->addFieldMapping('field_tags:create_term')->defaultValue(TRUE);
    $this->addFieldMapping('field_language')->defaultValue('it');
    $this->addFieldMapping('field_public_access_level', 'private');
    $this->addFieldMapping('body', 'notes');
    $this->addFieldMapping('field_additional_info', 'additional_info_keys');
    $this->addFieldMapping('field_additional_info:second', 'additional_info_values')->defaultValue('');

  }

  /**
   * Creates list of fields for CKAN Dataset.
   */
  public function getCkanDatasetFields() {
    return array(
      "license_title" => "License Title",
      "maintainer" => "Maintaier",
      "relationships_as_object" => "",
      "private" => "Private",
      "maintainer_email" => "",
      "revision_timestamp" => "Revision Date",
      "id" => "UUID",
      "metadata_created" => "Created Date",
      "metadata_modified" => "Modified Date",
      "author" => "Author",
      "author_email" => "Author Email",
      "state" => "State",
      "version" => "Version",
      "creator_user_id" => "Author UUID",
      "type" => "Node Type",
      "resources" => "Resources",
      "num_resources" => "Number of Resources",
      "tag_names" => "Tags",
      "tracking_summary" => "Tracking Summary",
      "groups" => "Groups",
      "license_id" => "Licence ID",
      "relationships_as_subject" => "",
      "num_tags" => "Number of Tags",
      "organization" => "Organization",
      "name" => "Name slug",
      "isopen" => "Is Open (boolean)",
      "url" => "URL",
      "notes" => "Description",
      "owner_org" => "Owner Organization",
      "extras" => "Extras",
      "title" => "Title",
      "revision_id" => "Revision ID",
        // This doesn't actually exist but we are adding it later in prepareRow.
      "uid" => "User ID",
      "resource_ids" => "Resource IDS",
      "group_ids" => "Group IDS",
      "notes" => "Body Text",
    );
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    parent::prepareRow($row);

    foreach ($row->tags as $tag) {
      $row->_tags[] = (isset($tag->name)) ? $tag->name : $tag;
    }

    $row->additional_info_keys = array();
    $row->additional_info_values = array();

    if (isset($row->extras) && is_array($row->extras) && count($row->extras) > 0) {
      foreach ($row->extras as $extra) {
        if ($extra->value != '') {
          $row->additional_info_keys[] = $extra->key;
          $row->additional_info_values[] = substr($extra->value,0,255);
        }
      }
    }
    else {
      unset($row->additional_info_keys);
      unset($row->additional_info_values);
    }
  }

  /**
   * Implements prepare().
   */
  public function prepare($dataset_prepare, $row) {
    parent::prepare($dataset_prepare, $row);

    // Prepare group data.
    $row->group = $this->prepareRowGroup($row->organization);

    // Prepare resources data.
    $row->resources = $this->prepareRowResources($row->resources);
  }

  /**
   * Prepare list of resources that need to be imported based on the row data.
   *
   * @param object $resources_row_data
   *   Resources data present on row.
   *
   * @return array
   *   An array with all resources data ready to be imported or empty if
   *   no resources where found.
   */
  public function prepareRowResources($resources_row_data) {
    $resources = array();

    // dsm($resources_row_data);
    foreach ($resources_row_data as $resource_row_data) {
      $resource = $this->prepareResourceHelper(
              $resource_row_data->url, $resource_row_data->format, $resource_row_data->name, NULL, $resource_row_data->description
      );

      if ($resource) {
        $resources[] = $resource;
      }
    }

    return $resources;
  }

  /**
   * Prepare group that needs to be imported based on the row data.
   *
   * @param object $group_row_data
   *   Group data present on row.
   *
   * @return object
   *   An object value with the group data needed to import the group.
   */
  public function prepareRowGroup($group_row_data) {

    $group = new stdClass();
    $group->name = $group_row_data->title;

    return $group;
  }

}
