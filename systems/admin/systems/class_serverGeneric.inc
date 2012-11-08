<?php

/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2012  FusionDirectory

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

class serverGeneric extends workstationGeneric
{
  var $objectclasses = array("top", "goServer", "GOhard");

  /*!
   *  \brief The main function : information about attributes
   */
  static function getAttributesInfo ($word = NULL, $rdn = NULL)
  {
    if ($word === NULL) {
      $word = _('server');
    }
    if ($rdn === NULL) {
      $rdn = get_ou('serverRDN');
    }
    return parent::getAttributesInfo($word, $rdn);
  }

  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return array(
      "plShortName"   => _("Generic"),
      "plDescription" => _("Server generic"),
      "plSelfModify"  => FALSE,
      "plCategory"    => array("server" => array("description"  => _("Server"),
                                                 "objectClass"  => "goServer")),
      "plObjectType"  => array("server" => array("name"   => _("Server"),
                                                 "filter" => "objectClass=goServer")),
      "plProvidedAcls"  => parent::generatePlProvidedAcls(self::getAttributesInfo())
    );
  }

}

?>