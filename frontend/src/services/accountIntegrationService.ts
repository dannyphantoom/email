import { toast } from 'react-hot-toast';

const API_BASE_URL = 'http://localhost:8080';

export interface ConnectedAccount {
  id: string;
  type: 'email' | 'messenger';
  provider: string;
  email?: string;
  username?: string;
  isActive: boolean;
  lastSync: number;
  unreadCount?: number;
}

export interface UnifiedMessage {
  id: string;
  accountId: string;
  sender: string;
  recipient: string;
  subject: string;
  content: string;
  messageType: 'email' | 'message' | 'notification';
  timestamp: number;
  isRead: boolean;
  isImportant: boolean;
  attachments: string[];
  metadata: Record<string, string>;
  accountProvider: string;
}

export interface ConnectGmailRequest {
  email: string;
  password: string;
}

export interface ConnectGmailOAuth2Request {
  email: string;
  code: string;
}

export interface ConnectWhatsAppRequest {
  phoneNumber: string;
  password: string;
}

export interface ConnectWhatsAppWebRequest {
  phoneNumber: string;
  sessionId: string;
}

export interface ConnectTelegramRequest {
  phoneNumber: string;
  code: string;
}

export interface ConnectTelegramBotRequest {
  botToken: string;
  chatId: string;
}

class AccountIntegrationService {
  private getAuthHeaders(): HeadersInit {
    const token = localStorage.getItem('authToken');
    return {
      'Content-Type': 'application/json',
      ...(token && { 'Authorization': `Bearer ${token}` })
    };
  }

  private async makeRequest<T>(
    endpoint: string, 
    options: RequestInit = {}
  ): Promise<T> {
    const url = `${API_BASE_URL}${endpoint}`;
    const config: RequestInit = {
      headers: this.getAuthHeaders(),
      ...options
    };

    try {
      const response = await fetch(url, config);
      
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      
      const data = await response.json();
      
      if (!data.success) {
        throw new Error(data.message || 'Request failed');
      }
      
      return data.data || data;
    } catch (error) {
      console.error('API request failed:', error);
      throw error;
    }
  }

  // Get all connected accounts for the current user
  async getConnectedAccounts(): Promise<ConnectedAccount[]> {
    try {
      const accounts = await this.makeRequest<ConnectedAccount[]>('/integration/accounts', {
        method: 'GET'
      });
      return accounts;
    } catch (error) {
      console.error('Failed to fetch connected accounts:', error);
      toast.error('Failed to load connected accounts');
      return [];
    }
  }

  // Connect Gmail account
  async connectGmail(request: ConnectGmailRequest): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/gmail', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('Gmail account connected successfully!');
      return true;
    } catch (error) {
      console.error('Failed to connect Gmail:', error);
      toast.error('Failed to connect Gmail account. Please check your credentials.');
      return false;
    }
  }

  // Connect Gmail account via OAuth2
  async connectGmailOAuth2(request: ConnectGmailOAuth2Request): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/gmail/oauth2', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('Gmail account connected successfully via OAuth2!');
      return true;
    } catch (error) {
      console.error('Failed to connect Gmail via OAuth2:', error);
      toast.error('Failed to connect Gmail account via OAuth2. Please try again.');
      return false;
    }
  }

  // Get Gmail OAuth2 URL
  async getGmailOAuth2Url(): Promise<string> {
    try {
      const response = await this.makeRequest<{ url: string }>('/integration/gmail/oauth2/url', {
        method: 'GET'
      });
      return response.url;
    } catch (error) {
      console.error('Failed to get Gmail OAuth2 URL:', error);
      throw error;
    }
  }

  // Connect WhatsApp account
  async connectWhatsApp(request: ConnectWhatsAppRequest): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/whatsapp', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('WhatsApp account connected successfully!');
      return true;
    } catch (error) {
      console.error('Failed to connect WhatsApp:', error);
      toast.error('Failed to connect WhatsApp account. Please check your credentials.');
      return false;
    }
  }

  // Connect WhatsApp Web account
  async connectWhatsAppWeb(request: ConnectWhatsAppWebRequest): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/whatsapp/web', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('WhatsApp Web account connected successfully!');
      return true;
    } catch (error) {
      console.error('Failed to connect WhatsApp Web:', error);
      toast.error('Failed to connect WhatsApp Web account. Please check your session ID.');
      return false;
    }
  }

  // Connect Telegram account
  async connectTelegram(request: ConnectTelegramRequest): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/telegram', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('Telegram account connected successfully!');
      return true;
    } catch (error) {
      console.error('Failed to connect Telegram:', error);
      toast.error('Failed to connect Telegram account. Please check your credentials.');
      return false;
    }
  }

  // Connect Telegram Bot account
  async connectTelegramBot(request: ConnectTelegramBotRequest): Promise<boolean> {
    try {
      await this.makeRequest('/integration/connect/telegram/bot', {
        method: 'POST',
        body: JSON.stringify(request)
      });
      
      toast.success('Telegram Bot connected successfully!');
      return true;
    } catch (error) {
      console.error('Failed to connect Telegram Bot:', error);
      toast.error('Failed to connect Telegram Bot. Please check your bot token and chat ID.');
      return false;
    }
  }

  // Get unified messages from all connected accounts
  async getUnifiedMessages(): Promise<UnifiedMessage[]> {
    try {
      const messages = await this.makeRequest<UnifiedMessage[]>('/integration/messages', {
        method: 'GET'
      });
      return messages;
    } catch (error) {
      console.error('Failed to fetch unified messages:', error);
      toast.error('Failed to load messages');
      return [];
    }
  }

  // Manually sync a specific account
  async syncAccount(accountId: string): Promise<boolean> {
    try {
      await this.makeRequest('/integration/sync', {
        method: 'POST',
        body: JSON.stringify({ accountId })
      });
      
      toast.success('Account synced successfully!');
      return true;
    } catch (error) {
      console.error('Failed to sync account:', error);
      toast.error('Failed to sync account');
      return false;
    }
  }

  // Remove a connected account
  async removeAccount(accountId: string): Promise<boolean> {
    try {
      await this.makeRequest(`/integration/accounts/${accountId}`, {
        method: 'DELETE'
      });
      
      toast.success('Account removed successfully!');
      return true;
    } catch (error) {
      console.error('Failed to remove account:', error);
      toast.error('Failed to remove account');
      return false;
    }
  }

  // Toggle account status (enable/disable)
  async toggleAccountStatus(accountId: string, isActive: boolean): Promise<boolean> {
    try {
      await this.makeRequest(`/integration/accounts/${accountId}/status`, {
        method: 'PUT',
        body: JSON.stringify({ isActive })
      });
      
      toast.success(`Account ${isActive ? 'enabled' : 'disabled'} successfully!`);
      return true;
    } catch (error) {
      console.error('Failed to toggle account status:', error);
      toast.error('Failed to update account status');
      return false;
    }
  }
}

export const accountIntegrationService = new AccountIntegrationService(); 