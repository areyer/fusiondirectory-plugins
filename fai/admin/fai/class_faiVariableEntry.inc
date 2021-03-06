<?php

/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2003-2010  Cajus Pollmeier
  Copyright (C) 2011-2013  FusionDirectory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

class faiVariableEntry extends plugin
{
  /* attribute list for save action */
  var $ignore_account= TRUE;
  var $attributes   = array("cn","description","FAIvariableContent");
  var $objectclasses= array();

  var $orig_cn              = "";
  var $dn            = "";
  var $cn            = "";
  var $FAIvariableContent   = "";
  var $description   = "";
  var $status        = "new";
  var $parent        = NULL;
  var $FAIstate      = "";

  function faiVariableEntry (&$config, $dn= NULL,$object=false)
  {
    parent::__construct($config, $dn);
    if((isset($object['cn'])) && (!empty($object['cn']))){
      $this->orig_cn= $object['cn'];
      $this->dn=$object['dn'];
      foreach($object as $name=>$value){
        $oname = $name;
        $this->$oname=addslashes($value);
      }
    }else{
      $this->status = "new";
      $this->orig_cn       = false;
    }
  }

  function execute()
  {
  /* Call parent execute */
  parent::execute();

    /* Fill templating stuff */
    $smarty     = get_smarty();
    $display = "";

    foreach($this->attributes as $attrs){
      $smarty->assign($attrs,htmlentities (utf8_decode($this->$attrs)));
    }

    $tmp = $this->plInfo();
    foreach($tmp['plProvidedAcls'] as $name => $translated){
      $acl = $this->getacl($name, preg_match("/freeze/",$this->FAIstate));
      $smarty->assign($name."ACL",$acl);
    }

    $smarty->assign("freeze",preg_match("/freeze/",$this->FAIstate));
    $display.= $smarty->fetch(get_template_path('faiVariableEntry.tpl', TRUE));
    return($display);
  }


  /* Save data to object */
  function save_object()
  {
     if((isset($_POST['SubObjectFormSubmitted'])) && !preg_match("/freeze/", $this->FAIstate)){
      foreach($this->attributes as $attrs){
        if($this->acl_is_writeable($attrs)){
          if(isset($_POST[$attrs])){
            $this->$attrs = $_POST[$attrs];
          }else{
            $this->$attrs = "";
          }
        }
      }
    }
  }

  /* Check supplied data */
  function check()
  {
    /* Call common method to give check the hook */
    $message= plugin::check();

    if(isset($this->parent->SubObjects[$this->cn]) && $this->cn != $this->orig_cn){
      $message[] = msgPool::duplicated(_("Name"));
    }

    if($this->FAIvariableContent == "") {
      $message[]= msgPool::required(_("Content"));
    }

    $c = trim($this->cn);
    if($c == ""){
      $message[] = msgPool::required(_("Name"));
    }
    if(preg_match("/[^a-z0-9_\-]/i",$c)){
      $message[] = msgPool::invalid(_("Name"),$c,"/[a-z0-9_\-]/i");
    }

    return ($message);
  }

  function save()
  {
    $tmp=array();
    foreach($this->attributes as $attrs){
      $tmp[$attrs] = stripslashes( $this->$attrs);
    }

    if(($this->orig_cn)&&($tmp['cn']!=$this->orig_cn)){
      $tmp['remove']['from']  = $this->orig_cn;
      $tmp['remove']['to']    = $tmp['cn'];
    }

    $tmp['dn']      = $this->dn;
    $tmp['status']  = $this->status;
    return($tmp);
  }

  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return array(
      'plShortName'   => _('Variable entry'),
      'plDescription' => _('FAI variable entry '),
      'plSelfModify'  => FALSE,
      'plPriority'    => 23,
      'plCategory'    => array('fai'),

      'plProvidedAcls' => array(
        'cn'                => _('Name'),
        'description'       => _('Description'),
        'FAIvariableContent'=> _('Variable content')
      )
    );
  }
}
?>
