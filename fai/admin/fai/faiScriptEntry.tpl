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
              <input type='text' size="45" maxlength="80" value="{$description}" name="description">
{/render}
            </td>
          </tr>
        </table>
    </td>
    <td style="border-left: 1px solid rgb(160, 160, 160);">
           &nbsp;
        </td>
    <td valign="top">
        <h2><img class="center" alt="" src="plugins/fai/images/fai_script.png" title="{t}Properties{/t}">&nbsp;{t}Script attributes{/t}</h2>
        <table width="100%">
          <tr>
            <td>
              <LABEL for="FAIpriority">
              {t}Priority{/t}
              </LABEL>
            </td>
            <td>
{render acl=$FAIpriorityACL}
                <select id="FAIpriority" name="FAIpriority" title="{t}Choose a priority. Low values result in an earlier, high values in a later execution.{/t}">
                        {html_options options=$FAIprioritys selected=$FAIpriority}
                    </select>
{/render}
            </td>
          </tr>
        </table>
    </td>
  </tr>
</table>
<p class="seperator">&nbsp;</p>
<h2><img class="center" alt="" src="plugins/fai/images/fai_script.png" title="{t}Script attributes{/t}">&nbsp;
  <LABEL for="FAIscript">
    {t}Script{/t}
  </LABEL>
  </h2>
<table width="99%">
    <tr>
        <td>
{render acl=$FAIscriptACL}
            <textarea name="FAIscript" style="width:100%;height:300px;" id="FAIscript"
        rows="20" cols="120">{$FAIscript}</textarea>
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
  <input type="submit" name="ImportUpload" value="{t}Import script{/t}">
{/render}
{render acl=$FAIscriptACL}
  {$DownMe}
{/render}
</div>
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

