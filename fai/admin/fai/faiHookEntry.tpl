<input type="hidden" name="SubObjectFormSubmitted" value="1">
<table width="100%">
  <tr>
    <td valign="top" width="50%">
      <h2><img class="center" alt="" src="images/forward.png" title="{t}Generic{/t}">&nbsp;{t}Generic{/t}</h2>
        <table>
          <tr>
            <td>
              {t}Name{/t}{$must}&nbsp;
            </td>
            <td>
{render acl=$cnACL}
              <input type='text' value="{$cn}" size="45" name="cn">
{/render}
            </td>
          </tr>
          <tr>
            <td>
              {t}Description{/t}&nbsp;
            </td>
            <td>
{render acl=$descriptionACL}
              <input type='text' value="{$description}" size="45" name="description">
{/render}
            </td>
          </tr>
        </table>
    </td>
    <td style="border-left: 1px solid rgb(160, 160, 160);">
           &nbsp;
        </td>
    <td style="vertical-align:top">
        <h2><img class="center" alt="" src="plugins/fai/images/fai_hook.png" title="{t}Hook attributes{/t}">&nbsp;{t}Hook attributes{/t}</h2>
        <table width="100%">
          <tr>
            <td>
              <LABEL for="FAItask">
              {t}Task{/t}&nbsp;
              </LABEL>
{render acl=$FAItaskACL}
              <select id="FAItask" name="FAItask" title="{t}Choose an existing FAI task{/t}">
                {html_options values=$tasks output=$tasks selected=$FAItask}
              </select>
{/render}
            </td>
          </tr>
        </table>
    </td>
  </tr>
</table>


<p class="seperator">&nbsp;</p>
<h2><img class="center" alt="" src="plugins/fai/images/fai_hook.png" title="{t}Hook attributes{/t}">&nbsp;
    <LABEL for="FAIscript">
        {t}Script{/t}
    </LABEL>
    </h2>
<table width="99%">
    <tr>
        <td>
{render acl=$FAIscriptACL}
            <textarea name="FAIscript" style="width:100%;height:300px;" id="FAIscript"
        rows=20 cols=120>{$FAIscript}</textarea>
{/render}
        </td>
    </tr>
</table>
<br>
<div>
{render acl=$FAIscriptACL}
    <input type="file" name="ImportFile">&nbsp;
{/render}
{render acl=$FAIscriptACL}
    <input type="submit" name="ImportUpload" value="{t}Import script{/t}" >
{/render}
{render acl=$FAIscriptACL}
  {$DownMe}
{/render}
</div>

<p class="seperator">&nbsp;</p>
<br>
<div style="align:right;" align="right">
{if !$freeze}
  <input type="submit" value="{msgPool type=applyButton}"   name="SaveSubObject">&nbsp;
{/if}
  <input type="submit" value="{msgPool type=cancelButton}"  name="CancelSubObject">
</div>
<!-- Place cursor -->
<script type="text/javascript">
  <!-- // First input field on page
  focus_field('cn','description');
  -->
</script>

