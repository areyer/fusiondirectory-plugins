<table width="100%">
  <tr>
    <td width="50%" valign="top">
        <h2><img class="center" alt="" src="plugins/fai/images/fai_small.png" title="{t}Generic{/t}">&nbsp;{t}Generic{/t}</h2>
        <table cellspacing="4">
          <tr>
            <td>
              <LABEL for="cn">
              {t}Name{/t}{$must}
              </LABEL>
            </td>
            <td>
{render acl=$cnACL}
              <input type='text' value="{$cn}"  maxlength="80" size="45" id="cn" name="cn"/>
{/render}
            </td>
          </tr>
          <tr>
            <td>
              <LABEL for="description">
              {t}Description{/t}
              </LABEL>
            </td>
            <td>
{render acl=$descriptionACL}
              <input type='text' size="45" maxlength="80" value="{$description}" name="description" id="description" >
{/render}
            </td>
          </tr>
        </table>
    </td>
  </tr>
</table>
<p class="seperator">&nbsp;</p>
<table width="100%">
  <tr>
    <td>
        <h2><img class="center" alt="" src="plugins/fai/images/fai_script.png" title="{t}Objects{/t}">&nbsp;
            {t}List of scripts{/t}
        </h2>
        <table width="100%" style="border:1px solid #B0B0B0; " cellspacing=0 cellpadding=0>
        <tr>
          <td>
            {$Entry_divlist}
{if $sub_object_is_addable}
            <input type="submit" name="AddSubObject"     value="{msgPool type=addButton}" title="{msgPool type=addButton}" >
{else}
            <input type="submit" name="Dummy2"     value="{msgPool type=addButton}" title="{msgPool type=addButton}" disabled>
{/if}
          </td>
        </tr>
        </table>
    </td>
  </tr>
</table>
<input type="hidden" value="1" name="FAIscript_posted" >
<!-- Place cursor -->
<script type="text/javascript">
<!--
  focus_field("cn","description");
-->
</script>

