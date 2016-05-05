<?php

/**
 * Вспомогательная функция по  фильтрации строк в текстовом поле
 * 
 * 
 * @param type $text
 */
function _mcb_proxy_sites_filter(&$text) {
  $t = explode("\n", $text);
  foreach ($t as $key => $value) {
    // Если есть пустые строки - то уберем их    
    $len = mb_strlen($value);
    if ($len == 1) {
      unset($t[$key]);
      continue;
    }
    // Приводим к нижнему регистру
    $t[$key] = strtolower($value);
    $url = parse_url($value);
  }
  $out = serialize($t);

  return $out;
}

/**
 *  Очищаем URL 
 * 
 * @param type $text
 * @return type
 */
function _mcb_proxy_parse_url(&$str) {
  $url = parse_url($str);
  // Если определили домен тогда вернуть его
  if (isset($url['host'])) {
    return $url['host'];
  }
  if (isset($url['path'])) {
    return $url['path'];
  }
  $out = 'Neznamo - chto';
  return $out;
}

function _mcb_proxy_prepare_regexp(&$str) {
  $ar = unserialize($str);



  $patterns = array();
  $patterns[0] = '/\//';
  $patterns[1] = '/\./';  
  $replacements = array();
  $replacements[0] = '\/';
  $replacements[1] = '\.';    
  foreach ($ar as $key => $value) {
    $ar[$key] =  preg_replace($patterns, $replacements, $value);
  }
  

  
  $result = implode("\n", $ar);
  
  
  return $result;
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_user_add($form_id, &$form_state) {


  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  //drupal_set_title('Я тебя по IP вычислю');
  $form['user'] = array(
    '#type' => 'fieldset',
    '#title' => 'Пользователь',
//    '#attributes' => array('class' => array('f_bss')),
    '#group' => 'vt',
  );

  $form['user']['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login',
    '#size' => 40,
    '#maxlength' => 30,
  );
  $form['user']['context'] = array(
    '#type' => 'textfield',
    '#title' => 'Novell context',
    '#size' => 40,
    '#maxlength' => 30,
  );

  // Account settings.
  $form['sites'] = array(
    '#type' => 'fieldset',
    '#title' => 'Фильтрация сайтов',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 200,
  );

  $form['sites']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );

  $form['sites']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );


  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

function mcb_proxy_form_user_add_validate($form_id, &$form_state) {
  $dbg = 6;
}

/**
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_add_submit($form_id, &$form_state) {
  $dbg = 0;

  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
    'context' => strtolower($values['context']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }

  // Пишем в таблицу данные
  $id = db_insert('mcb_proxy_users')
      ->fields($data)
      ->execute();
}

/**
 *  Форма обновления  данных по пользователю
 * 
 * Implements hook_forms();
 */
function mcb_proxy_form_user_edit($form_id, &$form_state, $id_user) {

  $dbg = 0;

  $query = db_select('mcb_proxy_users', 'u')
      ->fields('u')
      ->condition('id', $id_user)
      ->execute();
  $result = $query->fetchAll();

  $novell_user = current($result);

  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  //drupal_set_title('Я тебя по IP вычислю');
  $form['user'] = array(
    '#type' => 'fieldset',
    '#title' => 'Пользователь',
//    '#attributes' => array('class' => array('f_bss')),
    '#group' => 'vt',
  );

  $form['user']['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login',
    '#default_value' => $novell_user->login,
    '#size' => 40,
    '#maxlength' => 30,
  );
  $form['user']['context'] = array(
    '#type' => 'textfield',
    '#title' => 'Novell context',
    '#default_value' => $novell_user->context,
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['user']['user_id'] = array(
    '#type' => 'hidden',
    '#value' => $id_user,
  );

  // Account settings.
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#default_value' => implode("\n", unserialize($novell_user->sfa)),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );

  $form['sites_sfa']['sfa_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' =>_mcb_proxy_prepare_regexp($novell_user->sfa),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );



  // Account settings.
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#default_value' => implode("\n", unserialize($novell_user->sfd)),
    '#rows' => 15,
  );

  $form['sites_sfd']['sfd_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' =>_mcb_proxy_prepare_regexp($novell_user->sfd),
    '#rows' => 15,
  );  
  

  $form['submit'] = array(
    '#value' => 'Обновить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

/**
 * Обновление данных по пользователю - нажали кнопку обновить
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_edit_submit($form_id, &$form_state) {
  $dbg = 0;
  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
    'context' => strtolower($values['context']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }

  db_update('mcb_proxy_users')
      ->fields($data)
      ->condition('id', $values['user_id'])
      ->execute();
}

/**
 *  Импорт пользователей из формата CSV
 * 
 */
function mcb_proxy_form_user_import() {

  $form['information'] = array(
    '#markup' => '<em>Необходимо скормить csv файл в следующем формате: novell login | novell Context | allow site </em>',
  );

// надо добавить опции csv  разделители экранирование символы

  $form['file'] = array(
    '#type' => 'file',
    '#title' => 'Список пользователей',
    '#description' => 'Выберите файл с расширением csv',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Отправить',
  );
  return $form;
}

/**
 *  Проверяем загруженный файл
 * 
 */
function mcb_proxy_form_user_import_validate() {
  $validators = array(
    'file_validate_extensions' => array('csv'), // Проверка на расширения    
  );
  // Загружаем файл в sites/default/files (по умолчанию)
  if ($file = file_save_upload('file', $validators, 'public://')) {
    $form_state['values']['file'] = $file; // передаём информацию о файле в ф-ю mymodule_form_submit()
  }
  else {
    form_set_error('file', 'Файл не был загружен');
  }
}

/**
 * Form submit callback.
 */
function mcb_proxy_form_user_import_submit($form, &$form_state) {
  $dbg = 0;
  $file = $form_state['values']['file'];
  $file->status = FILE_STATUS_PERMANENT; // Изменяем статус файла на "Постоянный"
  file_save($file); // Сохраняем новый статус
  // Читаем файл, готовим данные для Добавления в БД
  $pusers = array();
  $homepage = file_get_contents('http://www.example.com/');


  drupal_set_message('Картинка загружена');
}

/**
 *  
 * 
 */
function mcb_proxy_page_user_import() {
  $dbg = 0;
  $content = 'VAGA';
  $file = '/opt/drbd/www/domains/it.metcombank.net/SQUID_ARM_USERS_clear.csv';
  $data = file_get_contents($file);
  $csv = explode("\n", $data);

  $user_acl = array();

  // Устанавливает разделитель поля (только один символ).
  $csv_delimiter = ',';
  // Устанавливает символ ограничителя поля (только один символ).
  $csv_enclosure = '"';
  // Устанавливает экранирующий символ (только один символ). По умолчанию равен обратному слешу (\).
  $csv_escape = '';
  foreach ($csv as $row) {
    // Приводим к нижнему регистру
    $row = strtolower($row);
    $d = str_getcsv($row, $csv_delimiter, $csv_enclosure);
    $login = $d[0];

    if ($d[0] == "" || $d[1] == "")
      continue;

    $url = _mcb_proxy_parse_url($d[1]);
    $user_acl[$login][] = $url;
  }
  $dbg = 0;
  // Пишем в БД
  mcb_proxy_user_acl($user_acl);
  return $content;
}

/**
 * 
 * 
 * @param array $data = (
  login = array (
 *      url1,
 *      url2,
 *      ) 
 *  ) 
 * login = bocharov-ms.hqm 
 */
function mcb_proxy_user_acl($data) {

  foreach ($data as $key => $sites) {
    $t = explode('.', $key);
    $login = $t[0];
    $context = $t[1];
    // Массив полей для записи в таблицу
    $data = array(
      'login' => $login,
      'context' => $context,
      'inet' => MCB_PROXY_ALLOW_ONLY,
    );

    $data['sfa'] = serialize($sites);
    // !!! Необходимо проверить есть ли такой пользователь или нет ?  
    // UPDATE OR INSERT in DB
    $exist = db_select('mcb_proxy_users', 'mcb_u')
            ->fields('mcb_u')
            ->condition('login', $login)
            ->condition('context', $context)
            ->execute()->fetchAll();
    if ($exist) {

      db_update('mcb_proxy_users')
          ->fields($data)
          ->condition('id', $exist[0]->id)
          ->execute();
    }
    else {
      // Пишем в таблицу данные
      $id = db_insert('mcb_proxy_users')
          ->fields($data)
          ->execute();
    }
  }
}
