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

#include_once 'class.tree.php';
#include_once 'class.scanner.php';
#include_once 'class.semantics.php';

class Parser
{
  var $scanner_;
  var $script_;
  var $tree_;
  var $status_;
  var $registeredExtensions_;

  var $status_text;

  function parse($script)
  {
    $this->registeredExtensions_ = array();
    $this->status_text = "incomplete";

    $this->script_ = $script;
    $this->tree_ = new Tree(Scanner::scriptStart());
    $this->tree_->setDumpFunc(array(&$this, 'dumpToken_'));
    $this->scanner_ = new Scanner($this->script_);
    $this->scanner_->setCommentFunc(array($this, 'comment_'));

    if ($this->commands_($this->tree_->getRoot()) &&
        $this->scanner_->nextTokenIs('script-end'))
    {
      return $this->success_('success');
    }

    return $this->status_;
  }

  function dumpParseTree()
  {
    return $this->tree_->dump();
  }

  function dumpToken_(&$token)
  {
    if (is_array($token))
    {
      $str = "<" . $token['text'] . "> ";
      foreach ($token as $k => $v)
      {
        $str .= " $k:$v";
      }
      return $str;
    }

    return strval($token);
  }

  function getPrevTokenText_($parent_id)
  {
    $childs = $this->tree_->getChilds($parent_id);

    for ($i=count($childs); $i>0; --$i)
    {
      $prev = $this->tree_->getNode($childs[$i-1]);

      if (in_array($prev['text'], array('{', '(', ',')))
      {
        // use command owning a block or list
        $prev = $this->tree_->getNode($parent_id);
      }

      if ($prev['class'] != 'comment')
      {
        return $prev['text'];
      }
    }

    $prev = $this->tree_->getNode($parent_id);
    return $prev['text'];
  }

  function getSemantics_($token_text)
  {
    $semantics = new Semantics($token_text);
    $semantics->setExtensionFuncs(array(&$this, 'registerExtension_'), array(&$this, 'isExtensionRegistered_'));
    return $semantics;
  }

  function registerExtension_($extension)
  {
    array_push($this->registeredExtensions_, str_replace('"', '', $extension));
  }

  function isExtensionRegistered_($extension)
  {
    return (in_array($extension, $this->registeredExtensions_) ? true : false);
  }

  function success_($text = null)
  {
    if ($text != null)
    {
      $this->status_text = $text;
    }

    return $this->status_ = true;
  }

  function error_($text, $token = null)
  {
    if ($token != null)
    {
      $text = 'line '. $token['line'] .': '. $token['class'] . " where $text expected near ". $token['text'];
    }

    $this->status_text = $text;
    return $this->status_ = false;
  }

  function done_()
  {
    $this->status_ = true;
    return false;
  }

  /*******************************************************************************
   * methods for recursive descent start below
   */

  function comment_($token)
  {
    $this->tree_->addChild($token);
  }

  function commands_($parent_id)
  {
    while ($this->command_($parent_id))
      ;

    return $this->status_;
  }

  function command_($parent_id)
  {
    if (!$this->scanner_->nextTokenIs('identifier'))
    {
      if ($this->scanner_->nextTokenIs(array('block-end', 'script-end')))
      {
        return $this->done_();
      }
      return $this->error_('identifier', $this->scanner_->peekNextToken());
    }

    // Get and check a command token
    $token = $this->scanner_->nextToken();
    $semantics = $this->getSemantics_($token['text']);
    if (!$semantics->validCommand($this->getPrevTokenText_($parent_id), $token['line']))
    {
      return $this->error_($semantics->message);
    }

    // Process eventual arguments
    $this_node = $this->tree_->addChildTo($parent_id, $token);
    if ($this->arguments_($this_node, $semantics) == false)
    {
      return false;
    }

    $token = $this->scanner_->nextToken();
    if ($token['class'] != 'semicolon')
    {
      if (!$semantics->validToken($token['class'], $token['text'], $token['line']))
      {
        return $this->error_($semantics->message);
      }

      if ($token['class'] == 'block-start')
      {
        $this->tree_->addChildTo($this_node, $token);
        $ret = $this->block_($this_node, $semantics);
        return $ret;
      }

      return $this->error_('semicolon', $token);
    }

    $this->tree_->addChildTo($this_node, $token);
    return $this->success_();
  }

  function arguments_($parent_id, &$semantics)
  {
    while ($this->argument_($parent_id, $semantics))
      ;

    if ($this->status_ == true)
    {
      $this->testlist_($parent_id, $semantics);
    }

    return $this->status_;
  }

  function argument_($parent_id, &$semantics)
  {
    if ($this->scanner_->nextTokenIs(array('number', 'tag')))
    {
      // Check if semantics allow a number or tag
      $token = $this->scanner_->nextToken();
      if (!$semantics->validToken($token['class'], $token['text'], $token['line']))
      {
        return $this->error_($semantics->message);
      }

      $this->tree_->addChildTo($parent_id, $token);
      return $this->success_();
    }

    return $this->stringlist_($parent_id, $semantics);
  }

  function stringlist_($parent_id, &$semantics)
  {
    if (!$this->scanner_->nextTokenIs('left-bracket'))
    {
      return $this->string_($parent_id, $semantics);
    }

    $token = $this->scanner_->nextToken();
    if (!$semantics->startStringList($token['line']))
    {
      return $this->error_($semantics->message);
    }
    $this->tree_->addChildTo($parent_id, $token);

    while ($token['class'] != 'right-bracket')
    {
      if (!$this->string_($parent_id, $semantics))
      {
        return $this->status_;
      }

      $token = $this->scanner_->nextToken();

      if ($token['class'] != 'comma' && $token['class'] != 'right-bracket')
      {
        return $this->error_('comma or closing bracket', $token);
      }

      $this->tree_->addChildTo($parent_id, $token);
    }

    $semantics->endStringList();
    return $this->success_();
  }

  function string_($parent_id, &$semantics)
  {
    if (!$this->scanner_->nextTokenIs(array('quoted-string', 'multi-line')))
    {
      return $this->done_();
    }

    $token = $this->scanner_->nextToken();
    if (!$semantics->validToken('string', $token['text'], $token['line']))
    {
      return $this->error_($semantics->message);
    }

    $this->tree_->addChildTo($parent_id, $token);
    return $this->success_();
  }

  function testlist_($parent_id, &$semantics)
  {
    if (!$this->scanner_->nextTokenIs('left-parant'))
    {
      return $this->test_($parent_id, $semantics);
    }

    $token = $this->scanner_->nextToken();
    if (!$semantics->validToken($token['class'], $token['text'], $token['line']))
    {
      return $this->error_($semantics->message);
    }
    $this->tree_->addChildTo($parent_id, $token);

    while ($token['class'] != 'right-parant')
    {
      if (!$this->test_($parent_id, $semantics))
      {
        return $this->status_;
      }

      $token = $this->scanner_->nextToken();

      if ($token['class'] != 'comma' && $token['class'] != 'right-parant')
      {
        return $this->error_('comma or closing paranthesis', $token);
      }

      $this->tree_->addChildTo($parent_id, $token);
    }

    return $this->success_();
  }

  function test_($parent_id, &$semantics)
  {
    if (!$this->scanner_->nextTokenIs('identifier'))
    {
      // There is no test
      return $this->done_();
    }

    // Check if semantics allow an identifier
    $token = $this->scanner_->nextToken();
    if (!$semantics->validToken($token['class'], $token['text'], $token['line']))
    {
      return $this->error_($semantics->message);
    }

    // Get semantics for this test command
    $this_semantics = $this->getSemantics_($token['text']);
    if (!$this_semantics->validCommand($this->getPrevTokenText_($parent_id), $token['line']))
    {
      return $this->error_($this_semantics->message);
    }

    $this_node = $this->tree_->addChildTo($parent_id, $token);

    // Consume eventual argument tokens
    if (!$this->arguments_($this_node, $this_semantics))
    {
      return false;
    }

    // Check if arguments were all there
    $token = $this->scanner_->peekNextToken();
    if (!$this_semantics->done($token['class'], $token['text'], $token['line']))
    {
      return $this->error_($this_semantics->message);
    }

    return true;
  }

  function block_($parent_id, &$semantics)
  {
    if ($this->commands_($parent_id, $semantics))
    {
      $token = $this->scanner_->nextToken();

      if ($token['class'] != 'block-end')
      {
        return $this->error_('closing curly brace', $token);
      }

      $this->tree_->addChildTo($parent_id, $token);
      return $this->success_();
    }
    return $this->status_;
  }
}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:

?>
