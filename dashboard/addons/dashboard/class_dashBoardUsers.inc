<?php
/*
 * This code is part of FusionDirectory (http://www.fusiondirectory.org)
 * Copyright (C) 2010 Antoine Gallavardin
 * Copyright (C) 2011-2013 FusionDirectory project
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */



class dashboardUsers extends simplePlugin
{
  function __construct(&$config, $dn = NULL, $object = NULL)
  {
    parent::__construct($config, $dn, $object);

    $this->stats    = $this->users_stats();
    $this->expired  = $this->expired_accounts_info();
  }

  function users_stats ()
  {
    /* User accounts statistics */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
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

    return array(
      'accounts' => array(
        array('name' => 'mail',   'nb' => $nb_mail_accounts,  'img' => 'geticon.php?context=applications&icon=internet-mail&size=16'),
        array('name' => 'posix',  'nb' => $nb_posix_accounts, 'img' => 'geticon.php?context=applications&icon=os-linux&size=16'),
        array('name' => 'samba',  'nb' => $nb_samba_accounts, 'img' => 'geticon.php?context=applications&icon=os-windows&size=16')
      ),
      'nb_accounts' => $nb_accounts,
      'user_img' => 'geticon.php?context=types&icon=user&size=16',
      'locked_accounts' => array('nb' => $nb_locked_accounts, 'img' => "geticon.php?context=status&icon=object-locked&size=16"),
    );
  }

  function expired_accounts_info ()
  {
    /*
     * Begin of code for selecting expired account
     */

    /* getting the date in TIMESTAMP UNIX format */
    $today        = floor(time() / 86400); // 24 * 60 * 60

    /* Fetch global value from fusiondirectory.conf */
    $next_expired_days = $this->config->get_cfg_value('dashboardExpiredAccountsDays', 15);
    $next_expired_date = ($today + $next_expired_days);

    /* search all account with all date, mail, telephone */
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->search('(&(|(objectClass=posixAccount)(objectClass=sambaSamAccount))(shadowExpire=*))',
                  array('uid','shadowExpire','mail','telephoneNumber','cn','manager'));

    $expired_accounts = array();
    $next_expired_accounts = array();
    while ($attrs = $ldap->fetch()) {
      // Test if account is expired now
      if ($attrs['shadowExpire'][0] <= $today) {
        $expired_accounts[] = $this->get_user_infos($attrs);
      } elseif ($attrs['shadowExpire'][0] <= $next_expired_date) {
        $next_expired_accounts[] = $this->get_user_infos($attrs);
      }
    }

    return array(
      'accounts'            => $expired_accounts,
      'accounts_next_days'  => $next_expired_accounts,
      'next_days'           => $next_expired_days,
    );
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

  static function getAttributesInfo()
  {
    return array(
      'stats' => array(
        'name'  => _('Statistics'),
        'attrs' => array(new FakeAttribute('stats')),
        'template' => get_template_path('users_stats.tpl', TRUE, dirname(__FILE__)),
      ),
      'expired_accounts' => array(
        'name'  => _('Expired accounts'),
        'attrs' => array(new FakeAttribute('expired')),
        'template' => get_template_path('users_accounts.tpl', TRUE, dirname(__FILE__)),
      ),
    );
  }

  /* Return plugin informations for acl handling  */
  static function plInfo()
  {
    return array(
      'plShortName'   => _('Users'),
      'plDescription' => _('Statistics about users'),
      'plObjectType'  => array('dashboard'),

      'plProvidedAcls'    => array()
    );
  }
}
?>
