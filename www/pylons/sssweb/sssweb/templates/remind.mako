<%inherit file="base.mako"/>
  
${h.form(h.url.current(action='remind_submit', method='post'))}
<p>${_('Enter you login name and click Remind button. Link to activate new password will be sent to your registered e-mail account.')}</p>
<p>${_('Login')} ${h.text('username')}</p>
${h.submit('remind', _('Remind'))}
${h.end_form()}
</div>


