<?php

/**
 * @file
 * CCAvenue administration menu items.
 */

// Process Postbacks from uc_ccavenue.
function uc_ccavenue_response_handler() {
  include("libs/Crypto.php");
  $workingKey = variable_get('uc_ccavenue_working_key', '');//Working Key should be provided here.
  $encResponse = $_POST["encResp"];
  $uc_order_id = $_POST["orderNo"];
  $rcvdString = decrypt($encResponse,$workingKey); //Crypto Decryption used as per the specified working key.
  $order_status = "";
  $decryptValues = explode('&', $rcvdString);
  $dataSize = sizeof($decryptValues);
  for($i = 0; $i < $dataSize; $i++) {
    $information = explode('=',$decryptValues[$i]);
    if($i==1)	$tracking_id = $information[1];
    if($i==3)	$order_status = $information[1];
  }
  $_SESSION['complete_payment']['order_id'] =  $uc_order_id;
  $order = uc_order_load($uc_order_id);

  switch ($order_status) {
    case 'Success': 
      $query = array('no-cache' => '1');
      $comment = t('CCavenue Payment Gateway tracking ID: @tracking_id', array('@tracking_id' => $tracking_id));
      $email = $_SESSION['complete_payment']['email'];
      $source_node_id = $_SESSION['complete_payment']['node_id'];
      $debug_message = t('Your transaction is successful with Tracking ID: @tracking_id', array('@tracking_id' => $tracking_id));
      uc_payment_enter($order->order_id, 'CCavenue Payment Gateway', $order->order_total, $order->products[0]->order_uid, NULL, $debug_message);
      uc_cart_complete_sale($order);
      uc_order_comment_save($order->order_id, 0, t('Payment of @amount @currency submitted through myPaymentGateway.', array('@amount' => $order->order_total, '@currency' => $order->currency)), 'order', 'payment_received');

      uc_order_comment_save($order->order_id, 0, t('CCavenue Payment Gateway reported a payment of @amount @currency', array('@amount' => $order->order_total , '@currency' => $order->currency)));
      drupal_set_message(t('Transaction successful. Your payment has been processed and the invoice is sent to your email address.'));
      drupal_goto('<front>');
      break;
    case 'Aborted':
      $debug_message = t('The transaction is cancelled by customer with Tracking ID: @tracking_id', array('@tracking_id' => $tracking_id));
      uc_order_comment_save($order->order_id, 0, $debug_message, 'order', 'canceled' );
      webform_update_payment_flag($_SESSION['webform_data']['nid'], $_SESSION['webform_data']['sid'], 'canceled', $uc_order_id);
      unset($_SESSION['webform_data']);
      drupal_set_message(t('Your payment can’t be completed. Please try registering for the event again'), 'error'); 
      unset($_SESSION['cart_order']);
      unset($_SESSION['complete_payment']['type']);
      drupal_goto('<front>');
      break;
    case 'Failure':
      $debug_message = t('The transaction has been declined with Tracking ID: @tracking_id', array('@tracking_id' => $tracking_id));
      $message = t("Your payment failed with following error message: @Error", array('@Error' => $debug_message));
      uc_order_comment_save($order->order_id, 0, $message, 'admin');
      webform_update_payment_flag($_SESSION['webform_data']['nid'], $_SESSION['webform_data']['sid'], 'failed', $uc_order_id);
      unset($_SESSION['webform_data']);
      $warning_message = t('In case you are facing difficulty in making the online payment, you can avail the ‘on-site’ registration option where the registration fee can be paid at the venue.');
      drupal_set_message($warning_message, 'warning highlight-this');
      unset($_SESSION['complete_payment']['type']);
      unset($_SESSION['cart_order']);
      drupal_goto('<front>');
    break;
    default :
      unset($_SESSION['webform_data']);
      drupal_set_message('Security Error. Illegal access detected');
      unset($_SESSION['complete_payment']['type']);
      unset($_SESSION['cart_order']);
      drupal_goto('node/' . $_SESSION['complete_payment']['nid']);
  }
}
