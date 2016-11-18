# dkan_harvest_ckan
The module extends the DKAN harvester in order to use the CKAN data sources.
This module is created for DKAN version 1.13(alpha) or later.

### Usage

- Enable the module
- In /node/add/harvest-source select "CKAN (JSON full catalog)" in the Type select list.
- in Source URI http://your.ckan.org/api/3/action/current_package_list_with_resources
- Run the harvester as usual

### About the call
If necessary add the parameter *limit* to the call,
for example 
http://your.ckan.org/api/3/action/current_package_list_with_resources?limit=1000

For a documentation about the CKAN's action *current_package_list_with_resources* please see http://docs.ckan.org/en/latest/api/.

