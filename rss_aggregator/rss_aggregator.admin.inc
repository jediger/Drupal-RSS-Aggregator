<?php
// $Id$
/** 
 * @file 
 * Administration page for the rss aggregator module. 
 */

/**
* Form builder; 
*/
function rss_aggregator_admin_form($form, &$form_state) {
	$articles = rss_aggregator_parse_xml();
	$header = array(
    'title' => t('Title'),
    'type' => t('Destination Node Type'),
    'source' => t('Source'),
    'date' => t('Published'),
    'tags' => t('Tags'),
    /*'changed' => array('data' => t('Updated'), 'field' => 'n.changed', 'sort' => 'desc')*/
  );
  $options = array();
  foreach ($articles as $key => $article) {
  	if($article['nid']==0) {
	  	$options[$article['guid']] = array(
	      'title' => l($article['title'], $article['link'], array('attributes' => array('target'=>'_blank'))),
	      'type' => $article['type'],
	      'source' => $article['source'],
	      'date' => date('F j, Y, g:i a' ,$article['date']),
	      'tags' => implode(', ', $article['taxonomy']),
	      /*'changed' => format_date($node->changed, 'short'),*/
	    );
    }
  }
  $form['nodes'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No content available.'),
  );
	$form['#attached']['css'][] = drupal_get_path('module', 'rss_aggregator') . '/rss_aggregator.css';
	$form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Import & Publish'));
	$form['actions']['refresh'] = array(
		'#type' => 'submit',
		'#value' => t('Refresh Article Cache'),
		'#submit' => array('rss_aggregator_admin_form_refresh'),
	);
	return $form;
}

/**
* Form validator; 
*/
function rss_aggregator_admin_form_validate($form, &$form_state) {
	
}


/**
* Form Submission Handler; 
*/
function rss_aggregator_admin_form_submit($form, &$form_state) {
	//dpm($form_state);
	$nodes = $form_state['values']['nodes'];
	$articles = rss_aggregator_parse_xml();
	foreach($articles as $article) {
		if(isset($nodes[$article['guid']]) && $nodes[$article['guid']]!=0) {
			rss_aggregator_create_node($article);
		}
	}
	drupal_set_message(t('The nodes have been saved'));
}

/**
* Refresh Article Cache; 
*/
function rss_aggregator_admin_form_refresh($form, &$form_state) {
	$sources = rss_aggregator_get_sources();
	if(empty($sources))
		form_set_error('name', t('You must enter sources'));
	db_delete('rss_aggregator_xml')->execute();
	foreach($sources as $source) {
		$xml = rss_aggregator_fetch_xml($source['url']);
		db_insert('rss_aggregator_xml')->fields(array(
			'raid' => $source['raid'],
			'xml' => $xml,
		))->execute();
	}
}

/**
 * Parse XML from Database and retrieve articles
 */
function rss_aggregator_parse_xml() {
	$articles = array();
	$guids = db_query("SELECT entity_id, field_guid_value FROM {field_data_field_guid}");
	$existing = array();
	foreach($guids as $guid) {
		$existing[$guid->field_guid_value] = array(
			'guid' => $guid->field_guid_value,
			'nid' => $guid->entity_id,
		);
	}
	$result = db_query("SELECT * FROM {rss_aggregator_xml} as xml LEFT JOIN {rss_aggregator_sources} AS sources ON sources.raid = xml.raid");
	foreach ($result as $record) {
		$xml = $record->xml;
		$doc = new DOMDocument();
		$doc->loadXML($xml);
		foreach($doc->getElementsByTagName('item') as $node) {
			$taxonomy = array();
			foreach($node->getElementsByTagName('category') as $term) {
				$taxonomy[] = $term->nodeValue;
			}
			$guid = $node->getElementsByTagName('guid')->item(0)->nodeValue;
			$nid = 0;
			if(isset($existing[$guid])) {
				$nid = $existing[$guid]['nid'];
			}
			$articles[] = array ( 
				'title' => $node->getElementsByTagName('title')->item(0)->nodeValue,
				'guid' => $guid,
				'desc' => $node->getElementsByTagName('description')->item(0)->nodeValue,
				'link' => $node->getElementsByTagName('link')->item(0)->nodeValue,
				'date' => strtotime($node->getElementsByTagName('pubDate')->item(0)->nodeValue),
				'source' => $record->name,
				'type' => $record->type,
				'taxonomy' => $taxonomy,
				'nid' => $nid,
			);
		}

	}
	uasort($articles, 'rss_aggregator_date_sort');
	return $articles;
}

function rss_aggregator_date_sort($a, $b) {
	if ($a['date'] == $b['date']) {
		return 0;
	}
	return ($a['date'] > $b['date']) ? -1 : 1;
}
