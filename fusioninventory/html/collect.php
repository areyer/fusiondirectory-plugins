<?php
# Collect inventory and store them in $dumpDir
# Usage :
#  fusioninventory-agent --server http://server/collect.php
require_once("../include/php_setup.inc");
require_once("functions.inc");
require_once("variables.inc");

$dumpDir = '/tmp';

##########################################################################
$http_raw_post_data = file_get_contents('php://input');

if (!$http_raw_post_data) {
    return;
}

$compressmode = 'none';
if (strpos($http_raw_post_data, "<?xml") === 0) {
    $xml = $http_raw_post_data;
} else if ($xml = @gzuncompress($http_raw_post_data)) {
    $compressmode = "gzcompress";
} else if ($xml = @gzinflate ("\x1f\x8b\x08\x00\x00\x00\x00\x00".$http_raw_post_data)) {
    // ** OCS agent 2.0 Compatibility, but return in gzcompress
    $compressmode = "gzdeflate";
    if (strstr($xml, "<QUERY>PROLOG</QUERY>")
            AND !strstr($xml, "<TOKEN>")) {
        $compressmode = "gzcompress";
    }
} else if ($xml = @gzinflate (substr($http_raw_post_data, 2))) {
    // ** OCS agent 2.0 Compatibility, but return in gzcompress
    $compressmode = "gzdeflate";
    if (strstr($xml, "<QUERY>PROLOG</QUERY>")
            AND !strstr($xml, "<TOKEN>")) {
        $compressmode = "gzcompress";
    }
} else {
    $f = tempnam('/tmp', 'gz_fix');
    file_put_contents($f, "\x1f\x8b\x08\x00\x00\x00\x00\x00".$http_raw_post_data);
    $xml = file_get_contents('compress.zlib://'.$f);

    unlink($f);

    if (strpos($xml, "<?xml") === 0) {
        $compressmode = "gzcompress";
    } else {
        $xml = '';
    }
}

if (strpos($xml, "<?xml") !== 0) {
    error_log("failed to extract XML content");
}

$reply = "";
if (preg_match('/QUERY>PROLOG<\/QUERY/', $xml)) {
    $reply= '<?xml version="1.0" encoding="UTF-8"?>
        <REPLY>
           <RESPONSE>SEND</RESPONSE>
           <PROLOG_FREQ>8</PROLOG_FREQ>
        </REPLY>';
} else {
    $reply = "<?xml version='1.0' encoding='UTF-8'?>
       <REPLY>
       </REPLY>";
    $invFile = sprintf('%s/%s.xml', $dumpDir, $_SERVER['REMOTE_ADDR']);

    $data = xml::xml2array($xml, 1);
    $data = $data['REQUEST']['CONTENT'];
    $cpus = $data['CPUS'];
    if (!is_numeric(key($cpus))) {
      $cpus = array($cpus);
    }
    $os = $data['OPERATINGSYSTEM'];
    if (!is_numeric(key($os))) {
      $os = array($os);
    }

    $macs = array();
    foreach ($data['NETWORKS'] as $network) {
      if (isset($network['MACADDR']) && ($network['MACADDR'] != "00:00:00:00:00:00")) {
        $macs[] = $network['MACADDR'];
      }
    }
    $macs = array_values(array_unique($macs));

    /* Check if CONFIG_FILE is accessible */
    if (!is_readable(CONFIG_DIR."/".CONFIG_FILE)) {
      die(sprintf(_("FusionDirectory configuration %s/%s is not readable. Aborted."), CONFIG_DIR, CONFIG_FILE));
    }

    /* Parse configuration file */
    $config = new config(CONFIG_DIR."/".CONFIG_FILE, $BASE_DIR);
    /* Set config server to default one */
    $directory = $config->data['MAIN']['DEFAULT'];
    if (!isset($config->data['LOCATIONS'][$directory])) {
      $directory = key($config->data['LOCATIONS']);
    }
    $config->set_current($directory);
    session::global_set('config', $config);
    $ldap = $config->get_ldap_link();

    $dn = 'cn='.$_SERVER['REMOTE_ADDR'].','.get_ou('inventoryRDN').$config->current['BASE'];
    $ldap->cat($dn);

  $msg = "";
    if ($ldap->count()) {
      /* Emtpy the subtree */
      $ldap->rmdir_recursive($dn);
      if (!$ldap->success()) {
        $msg.="error :".$ldap->get_error()."\n";
      }
    } else {
      /* Make sure branch is existing */
      $ldap->cd($config->current['BASE']);
      $ldap->create_missing_trees(get_ou('inventoryRDN').$config->current['BASE']);
    }
    /* Create root node */
    $ldap->cd($dn);
    $ldap->add(
      array(
        'cn'          => $_SERVER['REMOTE_ADDR'],
        'objectClass' => array('fdInventoryContent'),
        'macAddress'  => $macs,
      )
    );
    if (!$ldap->success()) {
      $msg.="error :".$ldap->get_error()."\n";
    }

    $infos = array(
      'CPUS' => array(
        'cn'    => 'cpu',
        'oc'    => 'fdInventoryCpu',
        'attrs' => array(
          'NAME','CORE','FAMILYNUMBER','MANUFACTURER',
          'MODEL','SPEED','STEPPING','THREAD'
        )
      ),
      'OPERATINGSYSTEM' => array(
        'cn'    => 'os',
        'oc'    => 'fdInventoryOperatingSystem',
        'attrs' => array(
          'NAME','FULL_NAME','VERSION',
          'KERNEL_NAME','KERNEL_VERSION'
        )
      ),
      'NETWORKS' => array(
        'cn'    => 'network',
        'oc'    => 'fdInventoryNetwork',
        'attrs' => array(
          'DESCRIPTION','IPADDRESS','IPMASK','IPSUBNET',
          'MACADDR','STATUS','VIRTUALDEV','DRIVER','PCISLOT',
          'IPADDRESS6','IPMASK6','IPSUBNET6',
        )
      )
    );

    foreach ($infos as $key => $info) {
      if (!isset($data[$key])) {
        continue;
      }
      $objects = $data[$key];
      if (!is_numeric(key($objects))) {
        $objects = array($objects);
      }
      foreach ($objects as $i => $object) {
        $cn         = $info['cn'].$i;
        $ldap_attrs = array(
          'cn' => $cn,
          'objectClass' => $info['oc'],
        );
        foreach ($info['attrs'] as $attr) {
          if (isset($object[$attr])) {
            $ldap_attrs['fdInventory'.preg_replace('/_/', '', $attr)] = $object[$attr];
          }
        }
        $msg.=print_r($ldap_attrs, TRUE);
        $ldap->cd('cn='.$cn.','.$dn);
        $ldap->add($ldap_attrs);
        if (!$ldap->success()) {
          $msg.="error :".$ldap->get_error()."\n";
        }
      }
    }

    if (!file_put_contents($invFile, "$dn\n$msg\n".print_r($data, TRUE))) {
        error_log("Failed to write ");
    }
}

switch($compressmode) {
    case 'none':
        print $reply;
        break;

    case 'gzcompress':
        print gzcompress($reply);
        break;

    case 'gzencode':
        print gzencode($reply);
        break;

    case 'gzdeflate':
        print gzdeflate($reply);
        break;

}
