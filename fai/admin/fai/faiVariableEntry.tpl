<input type="hidden" name="SubObjectFormSubmitted" value="1">
<table width="100%">
  <tr>
    <td valign="top" width="50%">
      <h2><img class="center" alt="" src="plugins/fai/images/fai_small.png" title="{t}Generic{/t}">&nbsp;{t}Generic{/t}</h2>
        <table>
          <tr>
            <td>
              {t}Name{/t}{$must}&nbsp;
            </td>
            <td>
{render acl=$cnACL}
              <input type='text' value="{$cn}" size="45" maxlength="80" name="cn">
{/render}
            </td>
          </tr>
          <tr>
            <td>
              {t}Description{/t}&nbsp;
            </td>
            <td>
{render acl=$descriptionACL}
              <input type='text' value="{$description}" size="45" maxlength="80" name="description">
{/render}
            </td>
          </tr>
        </table>
    </td>
    <td style="border-left: 1px solid rgb(160, 160, 160);">
           &nbsp;
        </td>
    <td valign="top">
        <h2><img class="center" alt="" src="plugins/fai/images/fai_variable.png" title="{t}Variable attributes{/t}">&nbsp;{t}Variable attributes{/t}</h2>
        <table  width="100%">
          <tr>
            <td>
              <LABEL for="Content">
              {t}Variable content{/t}{$must}&nbsp;
              </LABEL>
            </td>
            <td>
{render acl=$FAIvariableContentACL}
              <input type="text" name="FAIvariableContent" value="{$FAIvariableContent}" id="Content" style="width:250px;">
{/render}
            </td>
          </tr>
        </table>
    </td>
  </tr>
</table>
<p class="seperator">&nbsp;</p>
<br>
<div style="align:right;" align="right">
{if !$freeze}
  <input type="submit" value="{msgPool type=applyButton}"   name="SaveSubObject" >&nbsp;
{/if}
  <input type="submit" value="{msgPool type=cancelButton}"  name="CancelSubObject">
</div>

<!-- Place cursor -->
<script type="text/javascript">
  <!-- // First input field on page
  focus_field('cn','description');
  -->
</script>

