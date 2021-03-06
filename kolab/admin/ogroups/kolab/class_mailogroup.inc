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

class mailogroup extends plugin
{
  /* plugin specific values */
  var $mail= "";
  var $cn= "";

  /* attribute list for save action */
  var $attributes= array("mail");
  var $objectclasses= array("kolabGroupOfNames");
  var $members= array();
  var $view_logged = FALSE;

  function mailogroup (&$config, $dn= NULL, $parent= NULL)
  {
    parent::__construct($config, $dn, $parent);

    /* Include config object */
    $this->config= $config;

    /* Save initial account state */
    $this->initially_was_account= $this->is_account;
  }


  function execute()
  {
    /* Call parent execute */
    parent::execute();

    if($this->is_account && !$this->view_logged){
      $this->view_logged = TRUE;
      new log("view","ogroup/".get_class($this),$this->dn);
    }


    /* Do we need to flip is_account state? */
    if(isset($_POST['modify_state'])){
      if($this->is_account && $this->acl_is_removeable()){
        $this->is_account= FALSE;
      }elseif(!$this->is_account && $this->acl_is_createable()){
        $this->is_account= TRUE;
      }
    }

    /* Show tab dialog headers */
    if ($this->parent !== NULL){
      if ($this->is_account){
        $display= $this->show_disable_header(_("Remove mail account"),
            msgPool::featuresEnabled(_("mail group")));
      } else {
        $display= $this->show_enable_header(_("Create mail account"),
            msgPool::featuresDisabled(_("mail group")));
        return ($display);
      }
    }

    /* Initialize templating engine */
    $smarty= get_smarty();

    $tmp = $this->plInfo();
    foreach($tmp['plProvidedAcls'] as $name => $translation){
      $smarty->assign($name."ACL",$this->getacl("mail"));
    }

    /* Assign mail attribute */
    $smarty->assign("mail", $this->mail);

    /* Show main page */
    return ($display.$smarty->fetch (get_template_path('mail.tpl', TRUE,dirname(__FILE__))));
  }


  /* Check formular input */
  function check()
  {
    /* Call common method to give check the hook */
    $message= plugin::check();

    if ($this->is_account){
      $ldap= $this->config->get_ldap_link();

      /* Check if mail address is valid */
      if (!tests::is_email($this->mail) || $this->mail == ""){
        $message[]= msgPool::invalid(_("Mail address"),"","",_("your-name@your-domain.com"));
      }

      /* Check if mail address is already in use */
      $ldap->cd($this->config->current['BASE']);
      $ldap->search ("(&(!(objectClass=gosaUserTemplate))(|(mail=".$this->mail.")(gosaMailAlternateAddress=".$this->mail."))(!(cn=".$this->cn.")))", array("uid"));
      if ($ldap->count() != 0){
        $message[]= msgPool::duplicated(_("Mail address"));
      }
    }

    return ($message);
  }


  function save()
  {
    $ldap= $this->config->get_ldap_link();

    /* Call parents save to prepare $this->attrs */
    parent::save();

    /* Save data to LDAP */
    $ldap->cd($this->dn);
    $this->cleanup();
    $ldap->modify ($this->attrs);

    if($this->initially_was_account){
      new log("modify","ogroup/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
    }else{
      new log("create","ogroup/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
    }

    if (!$ldap->success()){
      msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()), LDAP_ERROR);
    }

    /* Optionally execute a command after we're done */
    if ($this->initially_was_account == $this->is_account){
      if ($this->is_modified){
        $this->handle_post_events("modify");
      }
    } else {
      $this->handle_post_events("add");
    }
  }


  /* remove object from parent */
  function remove_from_parent()
  {
    /* Cancel if there's nothing to do here */
    if (!$this->initially_was_account){
      return;
    }

    /* include global link_info */
    $ldap= $this->config->get_ldap_link();

    /* Remove and write to LDAP */
    parent::remove_from_parent();

    @DEBUG (DEBUG_LDAP, __LINE__, __FUNCTION__, __FILE__,
        $this->attributes, "Save");
    $ldap->cd($this->dn);
    $this->cleanup();
    $ldap->modify ($this->attrs);

    new log("remove","ogroup/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());

    if (!$ldap->success()){
      msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()), LDAP_ERROR);
    }
  }

   static function plInfo()
  {
    return array(
          "plShortName"   => _("Mail"),
          "plDescription" => _("Mail group"),
          "plSelfModify"  => FALSE,
          "plPriority"    => 4,
          "plCategory"    => array("ogroup"),
          "plProvidedAcls"=> array(
            "mail" => _("Mail address"))
          );
  }

}

// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>
