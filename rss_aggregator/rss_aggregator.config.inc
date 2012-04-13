<?php
// $Id$
/** 
 * @file 
 * Config page for the rss aggregator module. 
 */

/**
* Form builder; 
*/
function rss_aggregator_config_form($form, &$form_state) {
	$sources = rss_aggregator_get_sources();
	$types = node_type_get_types();
	$fields = rss_aggregator_get_guid_fields();
	$nodetypes = array();
	foreach($types as $key => $type) {
		$nodetypes[$key] = $type->name;
	}
	$form['table'] = array(
    '#tree' => TRUE,
    '#attributes' => array(
      'class' => array('twilio-map'),
    ),
    '#prefix' => '<table id="rss-aggregator-sources">',
    '#suffix' => '</table>',
  );
	$form['table']['header'] = array(
    '#prefix' => '<thead><tr>',
    '#suffix' => '</tr></thead>',
		'raid' => array(
			'#markup' => t('RAID'),
	    '#prefix' => '<th class="hide">',
	    '#suffix' => '</th>',
		),
		'name' => array(
			'#markup' => t('Source Name'),
	    '#prefix' => '<th>',
	    '#suffix' => '</th>',
		),
		'url' => array(
			'#markup' => t('Source URL'),
	    '#prefix' => '<th>',
	    '#suffix' => '</th>',
		),
		'type' => array(
			'#markup' => t('Node Type'),
	    '#prefix' => '<th>',
	    '#suffix' => '</th>',
		),
		'field' => array(
			'#markup' => t('GUID Field'),
	    '#prefix' => '<th>',
	    '#suffix' => '</th>',
		),
  );
  $odd = true;
  if(!empty($sources)) {
	  foreach($sources as $key => $source) {
			
			$form['table'][$key] = array(
		    '#prefix' => '<tr class="' . ($odd ? 'odd' : 'even') . '">',
		    '#suffix' => '</tr>',
		    'raid' => array(
		      '#type' => 'hidden',
		      '#default_value' => $source['raid'],
			    '#prefix' => '<td class="hide">',
			    '#suffix' => '</td>',
		    ),
		    'name' => array(
		      '#size' => 50,
		      '#type' => 'textfield',
		      '#default_value' => $source['name'],
			    '#prefix' => '<td>',
			    '#suffix' => '</td>',
		    ),
		    'url' => array(
		      '#size' => 150,
		      '#type' => 'textfield',
		      '#default_value' => $source['url'],
			    '#prefix' => '<td>',
			    '#suffix' => '</td>',
		    ),
		    'type' => array(
		      '#type' => 'select',
		      '#options' => $nodetypes,
		      '#default_value' => $source['type'],
			    '#prefix' => '<td>',
			    '#suffix' => '</td>',
		    ),
		    'field' => array(
		      '#type' => 'select',
		      '#options' => $fields,
		      '#default_value' => $source['field'],
			    '#prefix' => '<td>',
			    '#suffix' => '</td>',
		    ),
		  );
		  $odd = $odd ? false : true;
	  }
	}
  
  $form['table']['new'] = array(
    '#attributes' => array('class' => array('draggable', 'tabledrag-leaf', 'add-new')),
    '#row_type' => 'add_new_field',
    '#prefix' => '<tr class="draggable tabledrag-leaf ' . ($odd ? 'odd' : 'even') . '">',
    '#suffix' => '</tr>',
	    'raid' => array(
	      '#type' => 'hidden',
		    '#prefix' => '<td class="hide">',
		    '#suffix' => '</td>',
	    ),
	    'name' => array(
	      '#size' => 50,
	      '#title' => t('Source Name'),
	      '#type' => 'textfield',
		    '#prefix' => '<td>',
		    '#suffix' => '</td>',
	    ),
	    'url' => array(
	      '#size' => 150,
	      '#title' => t('Source URL'),
	      '#type' => 'textfield',
		    '#prefix' => '<td>',
		    '#suffix' => '</td>',
	    ),
	    'type' => array(
	      '#type' => 'select',
	      '#title' => t('Node Type'),
	      '#options' => $nodetypes,
		    '#prefix' => '<td>',
		    '#suffix' => '</td>',
	    ),
	    'field' => array(
	      '#type' => 'select',
	      '#title' => t('GUID Field'),
	      '#options' => $fields,
		    '#prefix' => '<td>',
		    '#suffix' => '</td>',
	    ),
  );
	$form['#attached']['css'][] = drupal_get_path('module', 'rss_aggregator') . '/rss_aggregator.css';
	$form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save'));
	return $form;
}

/**
* Form validator; 
*/
function rss_aggregator_config_form_validate($form, &$form_state) {
	$values = $form_state['values']['table'];
	foreach($values as $key => $row) {
		if($row['name']=='' && $row['url']=='') {
			//blank row
		} else {
			if($row['name']=='')
				form_set_error('name', t('You must enter a source name'));
			if($row['url']=='')
				form_set_error('name', t('You must enter a source url'));
			if(strpos($row['url'], 'http')===false)
				form_set_error('name', t('You must enter a valid source url'));
		}
	}
}


/**
* Form Submission Handler; 
*/
function rss_aggregator_config_form_submit($form, &$form_state) {
	$values = $form_state['values']['table'];
	foreach($values as $key => $row) {
		if(is_numeric($row['raid'])) {
			if($row['name']=='' && $row['url']=='') {
				db_delete('rss_aggregator_sources')
					->condition('raid', $row['raid'])
					->execute();
			} else {
				db_update('rss_aggregator_sources')
					->condition('raid', $row['raid'])
					->fields(array(
						'name' => $row['name'],
						'url' => $row['url'],
						'type' => $row['type'],
						'field' => $row['field'],
					))
					->execute();
			}
		} else if($row['name']!='' && $row['url']!='') {
			db_insert('rss_aggregator_sources')->fields(array(
				'name' => $row['name'],
				'url' => $row['url'],
				'type' => $row['type'],
				'field' => $row['field'],
			))->execute();
		}
	}
	drupal_set_message(t('The sources have been saved'));
}

function rss_aggregator_get_guid_fields() {
	$fields = db_query("SELECT field_name FROM {field_config} WHERE module = 'text'");
	$names = array();
	foreach($fields as $field) {
		$names[$field->field_name] = $field->field_name;
	}
	return $names;
}








