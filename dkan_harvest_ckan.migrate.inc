<?php

/**
 * @file dkan_harvest_ckan.migrate.inc
 *
 * Migration File for DKAN Harvest of CKAN file from
 *    /api/3/action/current_package_list_with_resources.
 *    or
 *    /api/3/action/package_list.
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

    $this->source = new HarvestMigrateSourceList(
            new HarvestList($this->dkanHarvestSource->getCacheDir()),
            new MigrateItemJSON($this->itemUrl),
            $this->getCkanDatasetFields(),
            $this->sourceListOptions
    );

  }

  /**
   * {@inheritdoc}
   */
  public function setFieldMappings() {
    parent::setFieldMappings();

    $this->addFieldMapping('uuid', 'id');
    $this->addFieldMapping('changed', 'metadata_modified');
    $this->addFieldMapping('field_modified_source_date', 'metadata_modified');
    $this->addFieldMapping('created', 'metadata_created');
    $this->addFieldMapping('field_contact_email', 'maintainer_email');
    $this->addFieldMapping('field_license', 'license_title');
    $this->addFieldMapping('field_tags', '_tags');
    $this->addFieldMapping('field_tags:create_term')->defaultValue(TRUE);
    $this->addFieldMapping('field_language')->defaultValue('it');
    $this->addFieldMapping('field_public_access_level', '_is_public');
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
    
    if(isset($row->type) && $row->type != 'dataset') {
      watchdog('dkan_harvest_ckan', 
  	    ('Dataset with type other from "dataset" - record id @id', array('id'=>$row->id)), $row->type, WATCHDOG_INFO);
      return false;
    }
    
    // Soft truncate the title
    if(strlen($row->title) > 255){
      watchdog('dkan_harvest_ckan', 
        t('Title too long cutted in record @id', array('id'=>$row->id)), $row->title, WATCHDOG_INFO);
      $row->title= mb_substr($row->title, 0, 251) . '...';
    }
    
    if(isset($row->private)){
      // Nota: il campo private riempie il campo field_public_access_level
      // quindi 0 diventa TRUE e 1 diventa FALSE.
      $row->_is_public = (intval($row->private) == 0) ? 'public' : 'private';
    }
    
    if(is_array($row->tags)){
      foreach ($row->tags as $tag) {
        $row->_tags[] = (isset($tag->name)) ? $tag->name : $tag;
      }
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
      // unset($row->spatial);
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
    if(!is_array($resources_row_data) && !is_object($resources_row_data)){
      return null;
    }
    
    foreach ($resources_row_data as $resource_row_data) {
      
      $resource_row_data->url=str_replace(" ", "%20", $resource_row_data->url);
      
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

    if(isset($group_row_data)){
        
      $group = new stdClass();
      $group->name = $group_row_data->title;
      
      if(isset($group_row_data->description)){
        $group->body = $group_row_data->description;
      }
      
      if(isset($group_row_data->image_url)){
        $group->field_image = $group_row_data->image_url;
      }
      

      return $group;
    }
    else {
      return null;
    }
      
  }
  
  
  /**
   * {@inheritdoc}
   */
  public function complete($dataset, $row) {

    // Create resources after the dataset was created.
    // If the creation of the dataset fails resources should not
    // be created. Also, we need the dataset to be created first
    // so the sync of groups work properly.
    $this->createResources($dataset, $row);

    // Add group.
    $this->prepareGroup($dataset, $row);

    // Save dataset.
    node_save($dataset);
  }
  
  
  /**
   * Get the nid of the group associated with the dataset.
   *
   * Create the group if it doesn't exist.
   * Add group nid to the row data.
   *
   * @param object $dataset
   *    Entity where groups are going to be added.
   * @param object $row
   *    Migration row.
   */
  private function prepareGroup(&$dataset, stdClass &$row) {

    // If there is a group check if it already exists and create it if not.
    if (isset($row->group)) {
      $group_data = $row->group;
      $gid = $this->getGroupIdByName($group_data->name);
      if (!$gid) {
        $group_node = $this->createGroupNode($group_data);
        if ($group_node) {
          $gid = $group_node->nid;
        }
      }

      if ($gid) {
        $dataset->og_group_ref[LANGUAGE_NONE][] = array(
          'target_id' => $gid,
        );
      }
    }
  }
  
  
  /**
   * Create a dkan group node from the values provided.
   *
   * @param array $group
   *    Values for the group node to be created.
   *
   * @return object
   *    Group node object.
   */
  public function createGroupNode($group) {

    $values = array(
      "type" => 'group',
      "is_new" => TRUE,
      "language" => LANGUAGE_NONE,
      "uid" => 1,
      "status" => NODE_PUBLISHED,
      "title" => $group->name,
    );
    
    $values['body'][LANGUAGE_NONE][0]['value'] = $group->body;
    
    // Image URL
    if(isset($group->field_image) 
        && $group->field_image != ''
        && substr($group->field_image,0,4) == 'http') {
      
      $file_temp_cont = file_get_contents($group->field_image);
      $file_temp = file_save_data($file_temp_cont, 'public://' . pathinfo($group->field_image, PATHINFO_FILENAME), FILE_EXISTS_RENAME);
      
      $values['field_image'][LANGUAGE_NONE][0] = array(
          'fid' => $file_temp->fid,
          'filename' => $file_temp->filename,
          'filemime' => $file_temp->filemime,
          'uid' => 1,
          'uri' => $file_temp->uri,
          'status' => 1,
        );
    }
    
    $group_node = entity_create('node', $values);
    node_save($group_node);
    
    watchdog(
      'dkan_harvest_ckan', 
      t('Organization @org created', array('@org'=>$group->name)), 
      (array) $group, 
      WATCHDOG_INFO
      );
    
    return $group_node;
  }
  
  /**
   * Create resources in a dataset.
   *
   * If we are adding a new dataset, create and add the nids to the
   * field_resources entity reference field.
   * If we are updating an existing dataset. Remove the existing resources and
   * replace them.
   *
   * @param object $dataset
   *        Entity where resources are going to be added.
   * @param object $row
   *        Migration row.
   */
  private function createResources(&$dataset, stdClass &$row) {
    // Delete all resources from dataset if any.
    // All resources will get imported again.
    $field_resources_ids = array();
    $dataset_old_emw = entity_metadata_wrapper('node', $dataset->nid);

    // Sometimes the field_resources would not be set before the node is saved.
    if (isset($dataset_old_emw->field_resources)) {
      foreach ($dataset_old_emw->field_resources->getIterator() as $delta => $resource_emw) {
        $field_resources_ids[] = $resource_emw->getIdentifier();
      }
    }

    if (!empty($field_resources_ids)) {
      entity_delete_multiple('node', $field_resources_ids);
    }

    if (isset($row->resources) && is_array($row->resources) && !empty($row->resources)) {
      // Create all resources and assign them to the dataset.
      $field_resources_value = array();
      foreach ($row->resources as $resource_row) {
        $resource_node = $this->createResourceNode($resource_row);
        // TODO add check?
        $field_resources_value[] = $resource_node->nid;
      }

      if (!empty($field_resources_value)) {
        foreach ($field_resources_value as $key => $resource_nid) {
          $dataset->field_resources[LANGUAGE_NONE][$key] = array(
            'target_id' => $resource_nid,
          );
        }
      }
    }
  }

  
}



class HarvestMigrationCKANPackages extends HarvestMigrationCKAN {
  
  public $itemUrl;

  /**
   * Constructor.
   */
  public function __construct($arguments) {
    
    parent::__construct($arguments);
    $this->itemUrl = str_replace("/package_list", '/' , $arguments['dkan_harvest_source']->uri)
            . 'package_show?id=:id';
    
    $this->source = new HarvestMigrateSourceList(
            new HarvestList($this->dkanHarvestSource->getCacheDir()),
            new CKANItemJSON($this->itemUrl),
            $this->getCkanDatasetFields(),
            $this->sourceListOptions
    );
    
  }
}


class HarvestMigrationCKANDataset extends HarvestMigrationCKAN {
  
  public $itemUrl;

  /**
   * Constructor.
   */
  public function __construct($arguments) {
    
    parent::__construct($arguments);
    $this->itemUrl = $arguments['dkan_harvest_source']->uri . '/:id';
    
    $this->source = new HarvestMigrateSourceList(
            new HarvestList($this->dkanHarvestSource->getCacheDir()),
            new MigrateItemJSON($this->itemUrl),
            $this->getCkanDatasetFields(),
            $this->sourceListOptions
    );
  }
  
  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    parent::prepareRow($row);

    // Fallback for the "title" on "name" (E.g. INPS)
    if(!isset($row->title)){
      $row->title = $row->name;
    }
  }
}
