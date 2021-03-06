<?php
/*
 * This code is part of FusionDirectory (http://www.fusiondirectory.org)
 * and  compatible with GOsa 2.6 (http://www.gosa-project.org)
 * Copyright (C) 2011 Alejandro Escanero Blanco (aescanero@gmail.com)
 * Copyright (C) 2011-2013  FusionDirectory
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

class CnNetgroupsAttribute extends DialogAttribute
{
  protected $dialogClass = "NetgroupSelectDialog";

  function addValue ($dn, $entry)
  {
    $value = $entry['cn'][0];
    if (!in_array($value, $this->value)) {
      $this->value[]    = $value;
    }
  }

  function getFilterBlackList ()
  {
    $cns    = $this->getValue();
    $cns[]  = $this->plugin->cn;
    return array('cn' => $cns);
  }
}

class netgroup extends simplePlugin
{
  var $objectclasses  = array("top", "nisNetgroup");
  var $mainTab        = TRUE;

  static function plInfo() {
    return array(
      'plShortName'   => _('Generic'),
      'plDescription' => _('Generic NIS Netgroup settings'),
      'plSelfModify'  => FALSE,
      'plObjectType'  => array('netgroup' => array(
        'name'    => _('NIS Netgroup'),
        'filter'  => 'objectClass=nisNetgroup',
        'ou'      => get_ou('netgroupRDN'),
        'icon'    => 'plugins/netgroups/images/iconMini.png'
      )),

      'plProvidedAcls' => parent::generatePlProvidedAcls(self::getAttributesInfo())
    );
  }

  // The main function : information about attributes
  static function getAttributesInfo ()
  {
    return array(
      'main' => array(
        'name'  => _('Informations'),
        'attrs' => array(
          new HostNameAttribute(
            _('Name'), _('Name of this NIS netgroup'), 'cn'
          ),
          new StringAttribute(
            _('Description'), _('Description of this NIS netgroup'), 'description'
          ),
          new BaseSelectorAttribute(get_ou('netgroupRDN'))
        )
      ),
      'users' => array(
        'name'  => _('User members'),
        'icon'  => 'geticon.php?context=types&icon=user&size=16',
        'attrs' => array(
          new SetAttribute(
            new StringAttribute('nisNetgroupTriple', '', 'nisNetgroupTriple')
          ),
          new UsersAttribute(
            '', _('NIS netgroup members'), 'memberUsers',
            FALSE, array(), 'dn', 'uid'
          ),
        )
      ),
      'systems' => array(
        'name'  => _('System members'),
        'icon'  => 'geticon.php?context=devices&icon=server&size=16',
        'attrs' => array(
          new SystemsAttribute('', _('NIS netgroup members'), 'memberSystems', FALSE),
        )
      ),
      'netgroups' => array(
        'name'  => _('Netgroup members'),
        'icon'  => 'plugins/netgroups/images/iconMini.png',
        'attrs' => array(
          new CnNetgroupsAttribute('', _('NIS netgroup members'), 'memberNisNetgroup'),
        )
      ),
    );
  }

  function __construct (&$config, $dn = NULL, $object = NULL)
  {
    parent::__construct($config, $dn, $object);

    $this->attributesAccess['cn']->setUnique(TRUE);
    $this->attributesAccess['nisNetgroupTriple']->setVisible(FALSE);
    $this->attributesAccess['memberUsers']->setInLdap(FALSE);
    $this->attributesAccess['memberSystems']->setInLdap(FALSE);
    $triples      = $this->attributesAccess['nisNetgroupTriple']->getValue();
    $usersfilter  = '(|';
    $cns          = array();
    foreach ($triples as $triple) {
      if (preg_match('/^\(([^,]*),([^,]*),[^)]*\)$/', $triple, $matches)) {
        if ($matches[1] != "") {
          $cns[] = $matches[1];
        } elseif ($matches[2] != "") {
          $usersfilter .= '(uid='.$matches[2].')';
        }
      }
    }
    $usersfilter .= ')';

    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->config->current['BASE']);
    $ldap->search('(&(objectClass=posixAccount)'.$usersfilter.')', array('uid'));
    $dns = array();
    while ($attrs = $ldap->fetch()) {
      $dns[] = $attrs['dn'];
    }
    $this->memberUsers    = $dns;
    $this->memberSystems  = $cns;
  }

  function prepare_save ()
  {
    $memberUsers    = $this->attributesAccess['memberUsers']->getDisplayValues();
    $memberSystems  = $this->memberSystems;
    $triples = array();

    foreach ($memberUsers as $uid) {
      $triples[] = '(,'.$uid.',)';
    }

    foreach ($memberSystems as $cn) {
      $triples[] = '('.$cn.',,)';
    }

    $this->nisNetgroupTriple = $triples;

    parent::prepare_save();
  }

  public function compute_dn()
  {
    return 'cn='.$this->cn.','.get_ou('netgroupRDN').$this->base;
  }
}
?>
