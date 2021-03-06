<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2003  Cajus Pollmeier
  Copyright (C) 2008  Gero Kuhlmann
  Copyright (C) 2008  Fabian Hickert
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

class mailMethodUwimap extends mailMethod {

  protected $ServerList   = array();
  protected $imap_handle  = NULL;
  protected $quota_loaded = FALSE;

  /* Allow modification of account_ids for existing mail accounts */
  protected $modifyableMail = FALSE;

  /* Allow modification of the mail server attribute existing mail accounts */
  protected $modifyableServer = FALSE;

  /* Enforces same value for 'mail' as used for 'cn' */
  protected $mailEqualsCN = FALSE;

  protected $enableDomainSelection  = FALSE;
  protected $enableQuota            = FALSE;
  protected $enableSieveManager     = FALSE;
  protected $enableVacationRange    = FALSE;
  protected $enableFolderTypes      = FALSE;

  /* the attribute used to create accounts */
  protected $uattrib = "uid";  // Naming attribute for accounts, e.g. imap.

  /* The account prefixes, keep the '.' here! See FAQ cyrusUseSlashes */
  protected $user_prefix  = "~.";
  protected $share_prefix = "share.";

  protected $procmaildir = "";

  protected function init()
  {
    mailMethod::init();
    if (isset($this->config->data['SERVERS']['IMAP'])) {
      $this->ServerList = $this->config->data['SERVERS']['IMAP'];
    }

    /* Check for required PROCMAIL_PATH configuration
     */
    if ($this->config->get_cfg_value("procMailPath", "") == "") {
      msg_dialog::display(_("Configuration error"), msgPool::invalidConfigurationAttribute("procMailPath"), ERROR_DIALOG);
      $this->procmaildir = "";
    } else {
      $this->procmaildir = $this->config->data['MAIN']['PROCMAIL_PATH'];
    }
  }


  public function saveSieveSettings()
  {
    if (empty($this->procmaildir)) {
      @DEBUG (DEBUG_MAIL, __LINE__, __FUNCTION__, __FILE__, "",
          "<b>MAIL</b>: Skippend writting scripts, 'procMailPath' is not configured");
      return;
    }

    mailMethod::saveSieveSettings();
    /* Map attribute from parent class
     */
    $mail                     = $this->parent->mail;
    $gosaMailDeliveryMode     = $this->parent->gosaMailDeliveryMode;
    $gosaMailAlternateAddress = $this->parent->gosaMailAlternateAddress;
    $gosaMailMaxSize          = $this->parent->gosaMailMaxSize;
    $gosaSpamMailbox          = $this->parent->gosaSpamMailbox;
    $gosaSpamSortLevel        = $this->parent->gosaSpamSortLevel;
    $gosaVacationMessage      = $this->parent->gosaVacationMessage;

    $addrlist = $mail;
    foreach ($gosaMailAlternateAddress as $val) {
      $addrlist .= "|$val";
    }
    $user = $this->getUAttribValue();

    /* Only create a new one, if it is not empty */
    if (is_integer(strpos($gosaMailDeliveryMode, "R")) ||
        is_integer(strpos($gosaMailDeliveryMode, "C")) ||
        is_integer(strpos($gosaMailDeliveryMode, "I")) ||
        is_integer(strpos($gosaMailDeliveryMode, "V")) ||
        is_integer(strpos($gosaMailDeliveryMode, "S"))) {
      $smarty = get_smarty();
      $smarty->left_delimiter   = '_{';
      $smarty->right_delimiter  = '}_';
      $smarty->assign('addrlist', $addrlist);
      $smarty->assign('mail', $mail);
      $smarty->assign('user', $user);

      $smarty->assign('spamfilter', is_integer(strpos($gosaMailDeliveryMode, "S")));
      $smarty->assign('spambox', $gosaSpamMailbox);
      $smarty->assign('spamlevel', $gosaSpamSortLevel);

      $smarty->assign('vacation', is_integer(strpos($gosaMailDeliveryMode, "V")));
      $smarty->assign('vacmsg', $gosaVacationMessage);

      $smarty->assign('discard', is_integer(strpos($gosaMailDeliveryMode, "I")));

      /* Write procmail script */
      $recipe = $smarty->fetch(get_template_path('procmail_script.tpl', TRUE, dirname(__FILE__)));
      $smarty->left_delimiter   = '{';
      $smarty->right_delimiter  = '}';
      $file = $this->procmaildir . "/" . $this->getUAttribValue();

      if (!file_put_contents($file, $recipe)) {
        msg_dialog::display(_("Error"), sprintf(_("Unable to write procmail script to '%s'."), $file), ERROR_DIALOG);
      }
    }
  }
}
?>
