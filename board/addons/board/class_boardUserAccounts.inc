<?php
/*
 * This code is part of FusionDirectory (http://www.fusiondirectory.org)
 * Copyright (C) 2003-2010 GONICUS GmbH
 * Copyright (C) 2010 Antoine Gallavardin
 * Copyright (C) 2011-2012 FusionDirectory project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



class boardUserAccounts extends plugin
{
  /* Generic */
  var $ui;
  var $view_logged    = FALSE;

  var $unix_account_expired       = array();
  var $unix_account_expired_week  = array();
  /* default values*/
  var $nb_expired_account               = 0;
  var $nb_expired_account_in_next_days  = 0;

  function __construct (&$config, $dn = NULL)
  {
    $this->config = &$config;
    $this->ui     = get_userinfo();
  }

  function execute()
  {
    /* Call parent execute */
    parent::execute();

    /* User accounts statistics */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd(get_ou("userRDN").$this->config->current['BASE']);
    $ldap->search("(&(!(objectClass=gosaUserTemplate))(objectClass=gosaAccount))", array("userPassword"));
    $nb_accounts = $ldap->count();
    $nb_locked_accounts = 0;
    while ($attrs = $ldap->fetch()) {
      if (isset($attrs['userPassword'][0]) && preg_match("/^\{[^\}]/", $attrs['userPassword'][0])) {
        if (preg_match("/^[^\}]*+\}!/", $attrs['userPassword'][0])) {
          $nb_locked_accounts++;
        }
      }
    }
    $ldap->search("(&(!(objectClass=gosaUserTemplate))(objectClass=gosaAccount)(objectClass=gosaMailAccount))", array("cn"));
    $nb_mail_accounts = $ldap->count();
    $ldap->search("(&(!(objectClass=gosaUserTemplate))(objectClass=gosaAccount)(objectClass=posixAccount))", array("cn"));
    $nb_posix_accounts = $ldap->count();
    $ldap->search("(&(!(objectClass=gosaUserTemplate))(objectClass=gosaAccount)(objectClass=sambaSamAccount))", array("cn"));
    $nb_samba_accounts = $ldap->count();

    /*
     * Begin of code for selecting expired account
     */

    /* getting the date in TIMESTAMP UNIX format */
    $today        = floor(time() / 86400); // 24 * 60 * 60

    /* Fetch global value from fusiondirectory.conf */
    $next_expired_days = $this->config->get_cfg_value("board_next_expired_account", "15");
    $next_expired_date = ($today + $next_expired_days);

    /* search all account with all date, mail, telephone */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->search("(&(|(objectClass=posixAccount)(objectClass=sambaSamAccount))(shadowExpire=*))",
                  array("uid","shadowExpire","mail","telephoneNumber","cn","manager"));

    $expired_accounts = array();
    $next_expired_accounts = array();
    while ($attrs = $ldap->fetch()) {
      // Test if account is expired now
      if ($attrs["shadowExpire"][0] <= $today) {
        $expired_accounts[] = $this->get_user_infos($attrs);
      } elseif ($attrs["shadowExpire"][0] <= $next_expired_date) {
        $next_expired_accounts[] = $this->get_user_infos($attrs);
      }
    }

    $smarty = get_smarty();
    /* assign search result */
    $smarty->assign("expired_accounts", $expired_accounts);
    $smarty->assign("expired_accounts_in_next_days", $next_expired_accounts);
    $smarty->assign("next_expired_days", $next_expired_days);

    $smarty->assign("nb_accounts", $nb_accounts);
    $smarty->assign("user_img", get_template_path("plugins/users/images/select_user.png"));
    $smarty->assign("nb_type_accounts", array(
      array('name' => 'mail',   'nb' => $nb_mail_accounts,  'img' => get_template_path("images/mailto.png")),
      array('name' => 'posix',  'nb' => $nb_posix_accounts, 'img' => get_template_path("images/penguin.png")),
      array('name' => 'samba',  'nb' => $nb_samba_accounts, 'img' => get_template_path("plugins/systems/images/select_winstation.png"))
    ));
    $smarty->assign("locked_accounts",
                    array('nb' => $nb_locked_accounts, 'img' => get_template_path("images/lists/locked.png")));

    return $smarty->fetch(get_template_path('user_accounts.tpl', TRUE));
  }

  function get_user_infos($attrs)
  {
    if (isset($attrs["manager"][0])) {
      $ldap_manager = $this->config->get_ldap_link();
      $ldap_manager->cd($this->config->current['BASE']);
      $manager_cn     = $ldap_manager->get_attribute($attrs["manager"][0], "cn");
      $manager_mail   = $ldap_manager->get_attribute($attrs["manager"][0], "mail");
      $manager_phone  = $ldap_manager->get_attribute($attrs["manager"][0], "telephoneNumber");
    } else {
      $manager_cn     = "";
      $manager_mail   = "";
      $manager_phone  = "";
    }

    $human_shadowExpire = date('d.m.Y', $attrs["shadowExpire"][0] * 86400); // 24 * 60 * 60

    return array(
      'uid' => $attrs["uid"][0],
      'cn'  => $attrs["cn"][0],
      'telephoneNumber' => (isset($attrs["telephoneNumber"][0])?$attrs["telephoneNumber"][0]:""),
      'mail' => (isset($attrs["mail"][0])?$attrs["mail"][0]:""),
      'manager_cn' => $manager_cn,
      'manager_mail' => $manager_mail,
      'manager_phone' => $manager_phone,
      'shadowExpire' => $human_shadowExpire,
    );
  }

  /* Return plugin informations for acl handling  */
  static function plInfo()
  {
    return array(
      "plShortName"   => _("Board - User"),
      "plDescription" => _("Statistics about users"),
      "plSelfModify"  => FALSE,
      "plPriority"    => 0,
      "plSection"     => array("addon" => _("Addons")),
      "plCategory"    => array("board"),

      "plProvidedAcls"    => array()
    );
  }
}
?>